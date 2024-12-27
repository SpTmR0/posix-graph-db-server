// cleanup.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define TERMINATION_MSG_TYPE 999

struct payload
{
    int seq_no;
    int op_no;
    char mtext[100];
}payload;
struct msg_buffer {
    long msg_type;
    struct payload payload;
}message;


int main() {
    key_t key;
    int msgid;

    // ftok to generate unique key
    key = ftok("gen.txt", 65);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // msgget creates a message queue and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    while (1) {
        printf("Want to terminate the application? Press Y (Yes) or N (No)\n");
        char input = getchar();

        if (input == 'Y' || input == 'y') {
            message.msg_type = 1;
            message.payload.op_no = -1;
            message.payload.seq_no - -1;
            strcpy(message.payload.mtext, "terminate");
            printf("%s\n", message.payload.mtext);

            // send termination message to the message queue
            if (msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1) {
                perror("msgsnd");
                exit(1);
            }

            break;
        }
    }

    return 0;
}