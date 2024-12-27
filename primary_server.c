// primary_server.c
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100
#define TERMINATION_MSG_OP 999
#define PRIMARY_SERVER 2

struct payload
{
    int seq_no;
    int op_no;
    char mtext[100];
};
struct msg_buffer {
    long msg_type;
    struct payload payload;
}message;


struct thread_arg
{
    int msgid;
    struct msg_buffer msg;
};

sem_t sem;

void *thread_run(void *v) {
    struct thread_arg *arg = (struct thread_arg *)v;
    int msgid = arg->msgid;
    struct msg_buffer *msg = &(arg->msg);

    key_t key = ftok("gen.txt", 65);
    if(key == -1) {
        perror("ftok");
        exit(1);
    }

    int shmid = shmget(key, 1024, 0666|IPC_CREAT);
    if(shmid == -1) {
        perror("shmget");
        exit(1);
    }

    char *str = (char*) shmat(shmid, (void*)0, 0);
    if(str == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    sem_wait(&sem);

    FILE* file = fopen(msg->payload.mtext, "w");
    if(file == NULL) {
        perror("fopen");
        pthread_exit(NULL);
    }

    int n = *str - '0';
    str++;

    fprintf(file, "%d\n", n);
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            int value = *str - '0';
            fprintf(file, "%d ", value);
            str++;
        }
        fprintf(file, "\n");        
    }
    

    fclose(file);

    sem_post(&sem);

    msg->msg_type = 5;
    strcpy(msg->payload.mtext, msg->payload.op_no == 1 ? "File added successfully" : "File changed successfully");
    printf("%s\n", msg->payload.mtext);
    if(msgsnd(msgid, msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
    }

    shmdt(str);

    pthread_exit(0);
}

int main() {
    key_t key = ftok("gen.txt", 65);
    if(key == -1) {
        perror("ftok");
        exit(1);
    }
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if(msgid == -1) {
        perror("msgget");
        exit(1);
    }

    if(sem_init(&sem, 0, 1) == -1) {
        perror("sem_init");
        exit(1);
    }

    struct thread_arg *arg = malloc(sizeof(struct thread_arg));
    arg->msgid = msgid;

    while(1) {

        if(msgrcv(msgid, &message, sizeof(message) - sizeof(long), 2, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        printf("Message received:%d %d %s\n", message.payload.op_no, message.payload.seq_no, message.payload.mtext);

        if(message.payload.op_no == -1){
            printf("Termination message received. Exiting\n");
            break;
        }

        if(message.payload.op_no == 1 || message.payload.op_no == 2) {
            pthread_t tid;
            pthread_attr_t attr;

            arg->msg = message;
            pthread_attr_init(&attr);
            if(pthread_create(&tid, &attr, thread_run, &arg) != 0){
                perror("pthread_ceate");
                exit(1);
            }
            if(pthread_join(tid, NULL) != 0){
                perror("pthread_join");
                exit(1);
            }
        }
    }

    sem_destroy(&sem);

    return 0;
}