#define _GNU_SOURCE

#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_MSG_SIZE 1024

int flag = 1;
void usage(char *progname) {
    printf("Usage: %s /queue_name\n", progname);
    exit(EXIT_FAILURE);
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("\n");
        flag = 0;
    }
}

void cleanup(mqd_t mqd) {
    if (mq_close(mqd) == -1) {
        perror("mq_close");
        exit(EXIT_FAILURE);
    }
}

void child_work(mqd_t mqd, char *buffer) {
    struct mq_attr attr;
    if (mq_getattr(mqd, &attr) == -1) {
        perror("mq_getattr");
        exit(EXIT_FAILURE);
    }
    attr.mq_flags = 0;
    mq_setattr(mqd, &attr, NULL);

    while (flag) {
        if (mq_getattr(mqd, &attr) == -1) {
            perror("mq_getattr");
            exit(EXIT_FAILURE);
        }
        if (attr.mq_curmsgs > 0) {
            printf("Message available\n");
            sleep(1);
            printf("Receiving message...\n");
            if (mq_receive(mqd, buffer, attr.mq_msgsize, 0) == -1) {
                perror("mq_receive");
                exit(EXIT_FAILURE);
            }
            sleep(1);
            printf("[%d] Child received: %s\n", getpid(), buffer);
        }
//        if (msgSize = mq_receive(mqd, buffer, attr.mq_msgsize, 0))
//        if(msgSize == -1)
//        {
//            printf("[%d] No message available\n", getpid());
//        }
//        else
//        {
//            printf("[%d] Child received: %s\n", getpid(), buffer);
//        }
//        sleep(1);
    }
}

void parent_work(mqd_t mqd, char *buffer) {
    struct mq_attr attr;
    while (flag) {
        if (mq_getattr(mqd, &attr) == -1) {
            perror("mq_getattr");
            exit(EXIT_FAILURE);
        }
        printf("[%d] Enter message to send to child: \n", getpid());
        fgets(buffer, attr.mq_msgsize, stdin);

        if (mq_send(mqd, buffer, attr.mq_msgsize, 0) == -1) {
            perror("mq_send");
            exit(EXIT_FAILURE);
        }
        sleep(5);
    }
}

int main(int argc, char **argv) {
    if (argc != 2 || argv[1][0] != '/') {
        usage(argv[0]);
    }
    char buffer[MAX_MSG_SIZE];
    printf("Buffer size: %d\n", sizeof(buffer));

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_flags = 0;

    mqd_t mq = mq_open(argv[1], O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &attr);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }
    signal(SIGINT, sig_handler);
    switch (fork()) {
        case -1:
            perror("fork");
            exit(1);
        case 0: {
            child_work(mq, buffer);
            printf("[%d] Child cleans and exiting...\n", getpid());
            cleanup(mq);
            exit(EXIT_SUCCESS);
        }
        default:
        {
            parent_work(mq, buffer);
        }
    }

    wait(NULL);
    printf("[%d] Parent cleans and exiting...\n", getpid());
    cleanup(mq);
    mq_unlink(argv[1]);
    exit(EXIT_SUCCESS);
}