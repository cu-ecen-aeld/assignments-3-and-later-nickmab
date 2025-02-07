#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_func_args->thread_complete_success = false;

    usleep(1000 * thread_func_args->wait_to_obtain_ms);

    int obtain_rc = pthread_mutex_lock(thread_func_args->mutex);
    if (obtain_rc != 0) {
        char* error_msg = "Unable to acquire lock";
        ERROR_LOG("%s, error code %d", error_msg, obtain_rc);
        thread_func_args->error_msg = error_msg;
        thread_func_args->error_code = obtain_rc;
    }

    usleep(1000 * thread_func_args->wait_to_release_ms);    

    int release_rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (release_rc != 0) {
        char* error_msg = "Unable to release lock";
        ERROR_LOG("%s, error code %d", error_msg, release_rc);
        thread_func_args->error_msg = error_msg;
        thread_func_args->error_code = release_rc;
    } else {
        thread_func_args->thread_complete_success = true;
        thread_func_args->error_msg = "success!";
        thread_func_args->error_code = 0;
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data* thread_param = malloc(sizeof(struct thread_data));

    if (thread_param == NULL) {
        perror("unable to alloc memory");
        exit(1);
    }

    thread_param->mutex = mutex;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;

    int rc = pthread_create(thread, NULL, threadfunc, thread_param);
    if (rc != 0)
    {
        ERROR_LOG("pthread_create failed with return code %d", rc);
        return false;
    }
    return true;
}

