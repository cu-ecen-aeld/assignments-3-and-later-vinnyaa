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

    // capture thread params
    struct thread_data *thread_data_parsed = (struct thread_data *) thread_param;

   
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // wait
    //printf("%d", thread_data_parsed->inactive_wait_ms);
    usleep(thread_data_parsed->inactive_wait_ms*1000); // 
    // lock mutex when it becomes available
    pthread_mutex_lock(thread_data_parsed->mutex_to_lock);

    // locking wait
    usleep(thread_data_parsed->locking_wait_ms * 1000);
    thread_data_parsed->thread_complete_success = true;
    pthread_mutex_unlock(thread_data_parsed->mutex_to_lock);

    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    // initialize mutex
    // pthread_mutex_init(mutex, NULL);

    struct thread_data* thread_param = (struct thread_data*)malloc(sizeof(struct thread_data));

    // initialize data struct for thread_data
    // struct thread_data thread_param;
    thread_param->inactive_wait_ms = wait_to_obtain_ms;
    thread_param->locking_wait_ms = wait_to_release_ms;
    thread_param->mutex_to_lock = mutex;

    int thread_id;
    thread_id = pthread_create(thread,NULL,threadfunc, (void *)thread_param);

    if (thread_id != 0) {
        free(thread_param);
        return false;
    }

    return true;

}

