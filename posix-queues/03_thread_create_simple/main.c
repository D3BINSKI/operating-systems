#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h> // for exit()
#include <unistd.h> // for sleep()

struct inout { // struct for passing data to threads
    int in;
    int out;
};

void pthread_create_wrapper(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
    int rc = pthread_create(thread, attr, start_routine, arg);
    if (rc) {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
}

void pthread_join_wrapper(pthread_t thread, void **value_ptr) {
    int rc = pthread_join(thread, value_ptr);
    if (rc) {
        printf("ERROR; return code from pthread_join() is %d\n", rc);
        exit(-1);
    }
}

void* thread_work_function(void* arg){
    printf("[%d] THREAD: working...\n", pthread_self());
    struct inout* io = (struct inout*)arg; // cast to struct inout
    io->out = io->in+1;
    printf("[%d] THREAD: done.\n", pthread_self());
    pthread_exit(NULL); // returns address of return value
}

int main(int argc, char** argv){
    pthread_t thread;
    struct inout io;
    io.in = 0;
    for (int i = 0; i < 5; ++i) {
        printf("io.in = %d\n", io.in);
        printf("[%d]MAIN: pthread_create\n", getpid());
        pthread_create_wrapper(&thread, NULL, thread_work_function, &io);
        printf("[%d]MAIN: created thread %d\n", getpid(),thread);
        printf("[%d]MAIN: pthread_join\n", getpid());
        pthread_join_wrapper(thread, NULL);
        printf("io.out = %d\n", io.out);
        io.in = io.out;
        sleep(1);
    }

    exit(EXIT_SUCCESS);
}
