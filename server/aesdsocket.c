#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

static const char* _PORT = "9000";
static const char* _OUTFILE = "/var/tmp/aesdsocketdata";

static int caught_shutdown_signal = 0;

void handle_signal(int sig_num) {
    if (caught_shutdown_signal != 0) {
        return;
    }
    switch (sig_num) {
        case SIGINT:
        case SIGTERM:
        caught_shutdown_signal = sig_num;    
            break;
        default:
            break;
    }
}

void init_signal_handler(void) {
    const int signals_to_handle[2] = { SIGINT, SIGTERM };
    int i = 0;
    for (; i < 2; i++) {
        sighandler_t ret = signal(signals_to_handle[i], (sighandler_t)handle_signal);
        if (ret == SIG_ERR) {
            int errnoCopy = errno;
            syslog(LOG_ERR, "Unable to install signal hadler for signal number %d. Errno: %d Error: %s",
                signals_to_handle[i], errnoCopy, strerror(errnoCopy));
            exit(-1);
        }
    }
}

/* Exits with code -1 if anything fails, otherwise returns socket file descriptor. */
int make_and_bind_socket(const char* port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        int errnoCopy = errno;
        syslog(LOG_ERR, "Cannot create socket. Errno: %d Error: %s",
                errnoCopy, strerror(errnoCopy));
        exit(-1);
    }

    struct addrinfo hints;
    struct addrinfo *my_addrinfo;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    const int ai_result = getaddrinfo(NULL, port, &hints, &my_addrinfo);
    if (ai_result != 0) {
        syslog(LOG_ERR, "Failed to getaddrinfo. Error: %s", gai_strerror(ai_result));
        exit(-1);
    }

    int bind_result = bind(fd, my_addrinfo->ai_addr, my_addrinfo->ai_addrlen);
    freeaddrinfo(my_addrinfo);

    if (bind_result != 0) {
        int errnoCopy = errno;
        syslog(LOG_ERR, "Cannot bind to port %s. Errno: %d Error: %s",
                port, errnoCopy, strerror(errnoCopy));
        exit(-1);
    }

    return fd;
}

void append_to_file(const char *data) {
    FILE *file = fopen(_OUTFILE, "a");

    if (file == NULL) {
        int errnoCopy = errno;
        syslog(LOG_ERR, "Cannot open file for writing: %s. Errno: %d Error: %s",
            _OUTFILE, errnoCopy, strerror(errnoCopy));
        exit(-1);
    }

    fwrite(data, 1, strlen(data), file);
    fclose(file);
}

void read_and_append_complete_packet(int peer_fd) {
    const size_t buf_size = 1024;
    char buf[buf_size];
    ssize_t total_bytes = 0;

    char finished = 0;
    while (!finished) {
        memset(buf, '\0', buf_size);
        ssize_t read_bytes = recv(peer_fd, buf, buf_size - 1, 0);

        if (read_bytes == -1) {
            int errnoCopy = errno;
            syslog(LOG_ERR, "recv failed from peer_fd %d. Errno: %d Error: %s",
                peer_fd, errnoCopy, strerror(errnoCopy));
            exit(-1);
        }

        total_bytes += read_bytes;
        ssize_t i = 0;
        for ( ; i < read_bytes; i++) {
            if (buf[i] == '\n') {
                finished = 1;
                buf[i+1] = '\0';
                break;
            }
        }

        if (read_bytes > 0) {
            append_to_file(buf);
        }
    }

    syslog(LOG_INFO, "Appended %zd bytes for peer fd %d to %s", total_bytes, peer_fd, _OUTFILE);
}

void spew_file_to_client(int peer_fd) {
    FILE *file = fopen(_OUTFILE, "r");

    if (file == NULL) {
        int errnoCopy = errno;
        syslog(LOG_ERR, "Cannot open file for reading: %s. Errno: %d Error: %s",
            _OUTFILE, errnoCopy, strerror(errnoCopy));
        exit(-1);
    }

    const size_t buf_size = 1024;
    char buf[buf_size];
    ssize_t total_bytes = 0;

    char finished = 0;
    while (!finished) {
        size_t bytes_read = fread(buf, 1, buf_size, file);
        if (feof(file)) {
            finished = 1;
        } else if (ferror(file) != 0) {
            syslog(LOG_ERR, "File read (%s) failed with error code %d",
                _OUTFILE, ferror(file));
            exit(-1);
        }

        total_bytes += bytes_read;
        if (bytes_read > 0) {
            send(peer_fd, buf, bytes_read, 0);
        }
    }

    fclose(file);
}

void handle_client(int peer_fd) {
    syslog(LOG_INFO, "Handling peer fd %d", peer_fd);
    read_and_append_complete_packet(peer_fd);
    spew_file_to_client(peer_fd);
}

void serve_until_stopped(int socket_fd) {
    const int listen_result = listen(socket_fd, 1);
    if (listen_result != 0) {
        int errnoCopy = errno;
        syslog(LOG_ERR, "Unable to listen on socket fd=%d. Errno: %d Error: %s",
            socket_fd, errnoCopy, strerror(errnoCopy));
        exit(-1);
    }

    /* still need to implement signal handling */
    struct sockaddr_in peer_addr;
    while (1) {
        socklen_t addrlen = sizeof(struct sockaddr);
        memset(&peer_addr, 0, addrlen);

        if (caught_shutdown_signal != 0) {
            return;
        }

        syslog(LOG_INFO, "Waiting to accept next connection...");
        const int peer_fd = accept4(socket_fd, (struct sockaddr *)&peer_addr, &addrlen, 0);
        if (peer_fd == -1) {
            int errnoCopy = errno;
            syslog(LOG_ERR, "Crashing because accept4 failed. Errno: %d Error: %s.",
                errnoCopy, strerror(errnoCopy));
            exit(-1);
        }

        char str[INET_ADDRSTRLEN];
        const char *peer_ip = inet_ntop(AF_INET, &peer_addr.sin_addr, str, INET_ADDRSTRLEN);

        syslog(LOG_INFO, "Accepted connection from %s", peer_ip);
        handle_client(peer_fd);
        close(peer_fd);
        syslog(LOG_INFO, "Closed connection from %s", peer_ip);
    }
}

int main(int /*argc*/, char **/*argv*/) {
    openlog("ass5-writer", LOG_PERROR | LOG_PID, LOG_DAEMON);
    init_signal_handler();
    const int fd = make_and_bind_socket(_PORT);
    syslog(LOG_INFO, "Bound to port %s, socket fd=%d. Now listening forever...\n", _PORT, fd);
    serve_until_stopped(fd);
    syslog(LOG_INFO, "Exiting gracefully after catching signal %d, deleting file %s",
        caught_shutdown_signal, _OUTFILE);
    close(fd);
    unlink(_OUTFILE);
    return 0;
}