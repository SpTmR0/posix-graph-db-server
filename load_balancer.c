// load_balancer.c
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TERMINATION_MSG_TYPE 999
#define TERMINATION_MSG_OP 999
#define PRIMARY_SERVER 2
#define SECONDARY_SERVER_1 3
#define SECONDARY_SERVER_2 4

struct payload
{
    int seq_no;
    int op_no;
    char mtext[100];
};
struct msg_buffer {
    long msg_type;
    struct payload payload;
};



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

    while(1){
        printf("\n");
        struct msg_buffer message;
        if(msgrcv(msgid, &message, sizeof(message) - sizeof(long), 1, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        printf("Message received: %d %d %s\n", message.payload.op_no, message.payload.seq_no, message.payload.mtext);

        if(message.payload.op_no == -1) {
            printf("Termination message receive. Exiting...\n");
            for(int i = 2; i <= 4; i++){
                message.msg_type = i;
                message.payload.op_no = -1;
                if(msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1){
                    perror("msgsnd");
                    exit(1);
                }
            }

            sleep(5);

            if(msgctl(msgid, IPC_RMID, NULL) == -1){
                perror("msgctl");
                exit(1);
            }
            break;
        }

        if(message.payload.op_no == 1 || message.payload.op_no == 2) {
            message.msg_type = 2;
            if(msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1) {
                perror("msgsnd");
                exit(1);
            }
            printf("Message sent to Primary server\n");
        } else if(message.payload.op_no == 3 || message.payload.op_no == 4) {
            if(message.payload.seq_no % 2 == 0) {
                message.msg_type = 4;
                if(msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1) {
                    perror("msgsnd");
                    exit(1);
                }
                printf("Message sent to secondary server 2\n");
            } else {
                message.msg_type = 3;
                if(msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1) {
                    perror("msgsnd");
                    exit(1);
                }
                printf("Message sent to secondary server 1\n");
            }
        }
    }

    return 0;
}