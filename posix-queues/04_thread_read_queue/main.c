#include <pthread.h>
#include <mqueue.h> // mq_open, mq_receive, mq_send, mq_close
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h> // for exit()
#include <unistd.h> // for sleep()
#include <signal.h> // for signal()

#define MAX_MSG_SIZE 1024

int flag = 1;

void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("\nSIGINT received. Exiting...\n");
        flag = 0;
    }
}

void pthread_create_wrapper(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
    printf("Creating thread\n");
    int rc = pthread_create(thread, attr, start_routine, arg);
    if (rc) {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
    printf("Thread created\n");
}

void pthread_join_wrapper(pthread_t thread, void **value_ptr) {
    printf("Joining thread\n");
    int rc = pthread_join(thread, value_ptr);
    if (rc) {
        printf("ERROR; return code from pthread_join() is %d\n", rc);
        exit(-1);
    }
}

void mq_attr_init_wrapper(struct mq_attr *attr) {
    attr->mq_flags = 0;
    attr->mq_maxmsg = 10;
    attr->mq_msgsize = MAX_MSG_SIZE;
}

void queue_create_wrapper(mqd_t *mq, struct mq_attr *attr, const char *name, int oflag, mode_t mode) {
    *mq = mq_open(name, oflag, mode, attr);
    if (*mq == -1) {
        printf("ERROR; return code from mq_open() is -1\n");
        exit(-1);
    }
}

void queue_reciver_wrapper(mqd_t* mq, char *msg_ptr, size_t msg_len, unsigned int *msg_prio) {
    int rc = mq_receive(*mq, msg_ptr, msg_len, msg_prio);
    if (rc == -1) {
        //for blocking flag
        printf("ERROR; return code from mq_receive() is -1\n");
        exit(-1);
    }
}

void queue_sender_wrapper(mqd_t* mq, const char *msg_ptr, size_t msg_len, unsigned int msg_prio) {
    int rc = mq_send(*mq, msg_ptr, msg_len, msg_prio);
    if (rc == -1) {
        printf("ERROR; return code from mq_send() is -1\n");
        exit(-1);
    }
}

void mqd_cleanup(mqd_t mq) {
    int rc = mq_close(mq);
    if (rc == -1) {
        printf("ERROR; return code from mq_close() is -1\n");
        exit(-1);
    }
}

void* thread_work_function(void* arg){
    mqd_t* mq = (mqd_t*)arg;
    char* buffer = (char*)malloc(MAX_MSG_SIZE*sizeof(char));
    struct mq_attr attr;
    mq_getattr(*mq, &attr);
    attr.mq_flags = 0;
    mq_setattr(*mq, &attr, NULL);
    while(flag){
        queue_reciver_wrapper(mq, buffer, MAX_MSG_SIZE, NULL);
        if(flag) printf("[%d]THREAD: %s", getpid(), buffer);
    }

    pthread_exit(NULL); // returns address of return value
}

void parent_work(mqd_t* mq)
{
    signal(SIGINT, sig_handler);
    char* buffer = (char*)malloc(MAX_MSG_SIZE*sizeof(char));
    while(flag){
        printf("[%d]PARENT: Enter message:", getpid());
        fgets(buffer, MAX_MSG_SIZE, stdin);
        queue_sender_wrapper(mq, buffer, MAX_MSG_SIZE, 0);
        nanosleep((const struct timespec[]){{0, 100000L}}, NULL);
    }
}


int main(int argc, char** argv) {
    pthread_t thread;
    struct mq_attr attr;
    mqd_t mq;

    signal(SIGINT, SIG_IGN);
    if(argc != 2){
        printf("Usage: %s <message queue name>\n", argv[0]);
        exit(-1);
    }

    mq_attr_init_wrapper(&attr);

    // create message queue
    queue_create_wrapper(&mq, &attr, argv[1], O_CREAT | O_RDWR, 0666);

    // create thread
    pthread_create_wrapper(&thread, NULL, thread_work_function, (void*)&mq);
    parent_work(&mq);
    pthread_join_wrapper(thread, NULL);
    mqd_cleanup(mq);
    mq_unlink(argv[0]);
    return 0;
}