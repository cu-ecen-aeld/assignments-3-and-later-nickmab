#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

void validate_args(int argc, char **argv) {
    if (argc != 3) {
        syslog(LOG_ERR, "Usage: %s <writefile> <writestr>", argv[0]);
        exit(1);
    }
}

FILE* open_file_or_die(char *writefile) {
    FILE *file = fopen(writefile, "w");

    if (file == NULL) {
        int errnoCopy = errno;
        syslog(LOG_ERR, "Cannot open file for writing: %s. Errno: %d Error: %s",
                writefile, errnoCopy, strerror(errnoCopy));
        exit(1);
    }

    return file;
}

void write_to_file_or_die(FILE *file, char *writestr) {
    /* file must be guarantee not NULL at this point. */
    size_t inByteCount = strlen(writestr);
    size_t outByteCount = fwrite(writestr, 1, inByteCount, file);

    if (inByteCount == outByteCount)
        return;

    int errnoCopy = errno;
    syslog(LOG_ERR, "Wrote %zu bytes to file instead of intended %zu bytes. Errno: %d Error: %s",
            outByteCount, inByteCount, errnoCopy, strerror(errnoCopy));
    exit(1);
}

int main(int argc, char **argv) {
    openlog("ass2-writer", LOG_PERROR, LOG_USER);
    validate_args(argc, argv);
    FILE *file = open_file_or_die(argv[1]);
    write_to_file_or_die(file, argv[2]);
    fclose(file);
    return 0;
}