// client.c
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>

#define PROJECT_CONSTANT 65
#define IPC_ERROR 1
#define SHM_ERROR 2
#define MSG_ERROR 3


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

    key_t key = ftok("gen.txt", PROJECT_CONSTANT);
    if(key == -1) {
        perror("ftok");
        return IPC_ERROR;
    }

    int msgid = msgget(key, 0666 | IPC_CREAT);
    if(msgid == -1) {
        perror("msgget");
        return MSG_ERROR;
    }

    while(1){
        int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
        if(shmid == -1) {
            perror("shmget");
            return SHM_ERROR;
        }

        char *str = (char*) shmat(shmid, (void*)0, 0);
        if(str == (void*)-1) {
            perror("shmat");
            return SHM_ERROR;
        }


        printf("1. Add a new graph to the database\n");
        printf("2. Modify an existing graph of the database\n");
        printf("3. Perform DFS on an existing graph of the database\n");
        printf("4. Perform BFS on an existing graph of the database\n");

        int op_no;
        printf("Enter Operation Number: ");
        scanf("%d", &op_no);

        int seq_no;
        printf("Enter Sequence Number: ");
        scanf("%d", &seq_no);

        char filename[50];
        printf("Enter Graph File Name: ");
        scanf("%s", filename);

        if(op_no == 1 || op_no == 2) {
            int n;
            printf("Enter the number of nodes of the graph: ");
            scanf("%d", &n);

            *str = '0' + n;
            str++;

            printf("Enter the adjacency matrix:\n");
            for(int i = 0; i < n; i++) {
                for(int j = 0; j < n; j++) {
                    int value;
                    scanf("%d", &value);
                    *str = '0' + value;
                    str++;
                }
            }
        } else if(op_no == 3 || op_no == 4) {
            int start_vertex;
            printf("Enter the start vertex: ");
            scanf("%d", &start_vertex);

            *str = '0' + start_vertex;
        }

        shmdt(str);
        if(shmdt == (void*)-1) {
            perror("shmdt");
            return SHM_ERROR;
        }

        struct msg_buffer message;
        message.msg_type = 1;
        message.payload.seq_no = seq_no;
        message.payload.op_no = op_no;
        strcpy(message.payload.mtext, filename);
        if(msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            return MSG_ERROR;
        }

        struct msg_buffer message_received;
        if(msgrcv(msgid, &message_received, sizeof(message_received) - sizeof(long), 5, 0) == -1) {
            perror("msgrcv");
            return MSG_ERROR;
        }
        printf("\n%s\n\n", message_received.payload.mtext);
        

    }

    return 0;
}