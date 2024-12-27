// secondary_server.c
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#define MAX 20
#define TERMINATION_MSG_OP 999

int visited[MAX];
sem_t sem;
char* result = "";
int f = 0;
int r = -1;
int q[20];


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

typedef struct Graph{
    int n;
    int** adj;
}Graph;


struct parent_thread_args
{
    int msgid;
    struct msg_buffer msg;
};

struct dfs_thread_args{
    int start;
    Graph graph;
};

Graph get_graph(char* filename){
    FILE* file = fopen(filename, "r");

    int n;
    fscanf(file, "%d", &n);

    int **adjacency_matrix = (int **)malloc(n * sizeof(int *));
    for(int i = 0; i < n; i++) {
        adjacency_matrix[i] = (int *)malloc(n * sizeof(int));
        for(int j = 0; j < n; j++) {
            fscanf(file, "%d", &adjacency_matrix[i][j]);
            printf("%d ", adjacency_matrix[i][j]);
        }
        printf("\n");
    }

    fclose(file);
    printf("Closed file.\n");

    Graph g;
    g.adj = adjacency_matrix;
    g.n = n;


    return g;
}

void BFS(int v, char* filename) {
    Graph graph = get_graph(filename);
    int n = graph.n;
    int **a = graph.adj;
	for (int i=1;i<=n;i++){
        if(a[v][i] && !visited[i]){
        q[++r]=i;
    }
    }
	if(f<=r) {
		visited[q[f]]=1;
		bfs(q[f++], graph);
	}
}

void DFS(int v, Graph graph);

void *thread_run_dfs(void *g) {
    struct dfs_thread_args *args = (struct dfs_thread_args *)g;
    Graph graph = args->graph;
    int start = args->start;
    int** adj = graph.adj;
    int n = graph.n;

    int is_leaf = 1;
    for(int i = 0; i < n; i++) {
        if(adj[start][i] == 1 && visited[i] == 0) {
            is_leaf = 0;
            DFS(i, graph);
        }
    }

    // If v is a leaf node, print it
    if(is_leaf) {
        int curr_vertex = start + 1;
        sem_wait(&sem);
        printf("%d ", start+1);
        char vertex = curr_vertex + '0';
        char* vertex_str = (char*)malloc(sizeof(char) * 3); // Allocate space for two characters and null terminator
        vertex_str[0] = vertex;
        vertex_str[1] = '\0';
        result = realloc(result, strlen(result) + strlen(vertex_str) + 1); // Allocate enough space for result
        strcat(result, vertex_str);
        sem_post(&sem);
        free(vertex_str);
    }

    free(args);
    pthread_exit(0);
}

void DFS(int v, Graph graph) {
    pthread_t tid;
    pthread_attr_t attr;

    printf("Created Semaphore.\n");
    sem_wait(&sem);
    visited[v] = 1;
    sem_post(&sem);

    struct dfs_thread_args *args = (struct dfs_thread_args *)malloc(sizeof(struct dfs_thread_args));
    args->graph = graph;
    args->start = v;

    pthread_create(&tid, &attr, thread_run_dfs, args);
    pthread_join(tid, NULL);
}

void *thread_run_parent(void *v){
    struct parent_thread_args *arg = (struct parent_thread_args *)v;
    int msgid = arg->msgid;
    struct msg_buffer *msg = &(arg->msg);

    key_t key = ftok("gen.txt", 65);
    if(key == -1) {
        perror("ftok");
        pthread_exit(NULL);
    }

    int shmid = shmget(key, 1024, 0666|IPC_CREAT);
    if (shmid == -1){
        perror("shmget");
        pthread_exit(NULL);
    }

    printf("Reading from shm\n");
    char *str = (char*) shmat(shmid, NULL, 0);
    if(str == (char*)-1) {
        perror("shmat");
        pthread_exit(NULL);
    }
    int start_vertex = *str - '0';
    //start_vertex += 1;
    printf("Start vertex:%d\n", start_vertex);

    char filename[100] = msg->payload.mtext;
    printf("\n%s\n", msg->payload.mtext);
    printf("Reached filename: %s\n", filename);
    Graph graph = get_graph(filename);


    for (int i = 0; i < MAX; i++)
    {
        visited[i] = 0;
    }
    
    printf("Initialized values\n");

    if(msg->payload.op_no == 3){
        result = "";
        printf("Starting DFS.\n");
        DFS(start_vertex, graph);
        strcpy(msg->payload.mtext, result);
        msg->msg_type = 5;
        printf("DFS done, sending message...\n");
        if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1)
        {
            perror("msgsnd");
            pthread_exit(NULL);
        }
        printf("DFS done, %s\n", result);
        result = "";
    }
    else if(msg->payload.op_no == 4){
        result = "";
        BFS(start_vertex, filename);
        for(int i = 0; i<20; i++){
            result += q[i] + '0';
        }
        strcpy(msg->payload.mtext, result);
        msg->msg_type = 5;
        if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1)
        {
            perror("msgsnd");
            pthread_exit(NULL);
        }
        result = "";
        
    }

    if (shmdt(str) == -1){
        perror("shmdt");
    }

    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <msg_type>\n", argv[0]);
        exit(1);
    }

    int msg_type = atoi(argv[1]);

    struct parent_thread_args *arg = (struct parent_thread_args *) malloc(sizeof(struct parent_thread_args));
    sem_init(&sem, 0, 1);

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
    arg->msgid = msgid;

    result = (char *)malloc(MAX * sizeof(char));
    if(result == NULL){
        perror("malloc");
        exit(1);
    }

    while(1) {
        msgrcv(msgid, &message, sizeof(message) - sizeof(long), msg_type, 0);
        printf("Message received: msgop:%d mtext:%s\n", message.payload.op_no, message.payload.mtext);

        if(message.payload.op_no == -1){
            printf("Termination message received. Exiting...\n");
            break;
        }
        if(message.payload.op_no == 3 || message.payload.op_no == 4) {
            pthread_t tid;
            pthread_attr_t attr;

            arg->msg = message;
            pthread_attr_init(&attr);
            if(pthread_create(&tid, &attr, thread_run_parent, &arg) != 0){
                perror("pthread_ceate");
                exit(1);
            }
            printf("here");
            printf("Created thread, waiting for execution to be finished\n");

            pthread_join(tid, NULL);
            printf("%s", result);

            free(result);
            result = (char*)malloc(MAX * sizeof(char));
            if(result == NULL){
                perror("malloc");
                exit(1);
            }
        }
    

    free(arg);
    free(result);
    sem_destroy(&sem);
    }
    return 0;
}