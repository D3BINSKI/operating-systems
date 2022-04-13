#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

void thread_routine(union sigval sv);

struct thread_info{
    mqd_t* mqCurr;
    mqd_t* mqNext;
    int id;
};

void set_mq_notify(mqd_t* mqCurr, mqd_t* mqNext, int id)
{
    struct thread_info *tinfo;
    tinfo = malloc(sizeof(struct thread_info));
    tinfo->mqCurr = mqCurr;
    tinfo->mqNext = mqNext;
    tinfo->id = id;

    struct sigevent not;
    memset(&not, 0, sizeof(not));
    not.sigev_notify = SIGEV_THREAD;
    not.sigev_notify_function = thread_routine;
    not.sigev_value.sival_ptr = tinfo;
    not.sigev_notify_attributes = NULL;
    if(mq_notify(*mqCurr, &not) == -1)
        ERR("mq_notify");
}

void thread_routine(union sigval sv)
{
    uint8_t n;
    unsigned *next_que;
    struct thread_info *tinfo;
    tinfo = (struct thread_info*)sv.sival_ptr;

    set_mq_notify(tinfo->mqCurr, tinfo->mqNext, tinfo->id);

    while(1)
    {
        if(mq_receive(*tinfo->mqCurr, (char*)&n, sizeof(n), NULL) == -1)
        {
            if(errno == EAGAIN) break;
            ERR("mq_receive");
        }
        if(n%tinfo->id == 0)
        {
            printf("q%d: %d\n", tinfo->id, n);
        }
        if(tinfo->mqNext != NULL && TEMP_FAILURE_RETRY(mq_send(*tinfo->mqNext,(char*)&n,1,NULL))<0)
            ERR("mq_send");
    }

}

void create_n_queues(mqd_t* q, struct mq_attr* attr, int n)
{
    for (int i = 0; i < n; ++i) {
        char name[20];
        snprintf(name, sizeof(name), "/queue%d", i+1);
        printf("creating queue %s\n",name);
        mq_unlink(name);
        q[i] = TEMP_FAILURE_RETRY(mq_open(name,O_RDWR|O_CREAT|O_NONBLOCK,0666,attr));
        if(q[i] == -1)
        {
            ERR("mq_open");
        }
    }
}

void close_n_queues(mqd_t* q, int n)
{
    for (int i = 0; i < n; ++i) {
        char name[20];
        printf("closing queue %d\n",i+1);
        snprintf(name, sizeof(name), "/queue%d", i+1);
        if(TEMP_FAILURE_RETRY(mq_close(q[i])) == -1)
        {
            ERR("mq_close");
        }
    }
}

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        fprintf(stderr,"Usage: %s <number of queues>(1<=n<=100)\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    int n;
    n = atoi(argv[1]);
    if(n<1 || n>100)
    {
        fprintf(stderr,"Usage: %s <number of queues>(1<=n<=100)\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    mqd_t q[n];
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1;

    create_n_queues(q,&attr,n);

    //set mq_notify
    for (int i = 0; i < n; ++i) {
        set_mq_notify(&q[i], ((i+1)%n==0)?NULL:&q[(i+1)%n], i+1);
    }

    char nr[20];
    int number;
    //read from stdin until EOF
    while(scanf("%s",&nr) == 1 && /*not EOF*/ nr[0] != '\0')
    {
        //check if nr is a number
        if(isdigit(*nr))
        {
            number = atoi(nr);
            if(TEMP_FAILURE_RETRY(mq_send(q[0],(const char*)&number,1,NULL))<0)
                ERR("mq_send");
        }
    }
    sleep(2);
    close_n_queues(q,n);


    exit(EXIT_SUCCESS);
}