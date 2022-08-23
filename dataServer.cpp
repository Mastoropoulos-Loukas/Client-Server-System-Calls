#include <iostream>
#include <sys/wait.h>	     /* sockets */
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */
#include <unistd.h>	         /* fork */		
#include <stdlib.h>	         /* exit */
#include <ctype.h>	         /* toupper */
#include <signal.h>          /* signal */
#include <stdio.h>
#include <arpa/inet.h>
#include <cstring>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <pthread.h>
#include <queue>

#include "util.h"


using namespace std;

int port;
int thread_pool_size;
int queue_size;
int block_size = 255;
char* buffer;

pthread_mutex_t mtx;
pthread_mutex_t socket_use;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;

typedef struct {
    int client_socket;
    char path[MAX_PATH_LEN];
} request;
queue<request> pool;

int readArguments(int argc, char** argv);
void printArguments();
int communicate(int sock, char data[]);
int work(char* path, int newsock, char buffer[]);

void work() {
	pthread_mutex_lock(&mtx);
	while (pool.empty()) {
        // printf("waiting to work...\n");
		pthread_cond_wait(&cond_nonempty, &mtx);
        // printf("worker unlocked...\n");
		}
    request req = pool.front(); pool.pop();
	pthread_mutex_lock(&socket_use);
	work(req.path, req.client_socket, buffer);
	pthread_mutex_unlock(&socket_use);
	pthread_mutex_unlock(&mtx);
}

int commune(int sock) {
	pthread_mutex_lock(&mtx);
	while (pool.size() >= queue_size) {
		// printf(">> Found Buffer full \n");
		pthread_cond_wait(&cond_nonfull, &mtx);
		}
	pthread_mutex_lock(&socket_use);
	communicate(sock, buffer);
	pthread_mutex_unlock(&socket_use);
	pthread_mutex_unlock(&mtx);
	return 1;
}

void * communicator(void * ptr)
{
    int *sock = (int*)ptr;
    commune(*sock);
    pthread_cond_broadcast(&cond_nonempty);
	pthread_exit(0);
}

void * worker(void * ptr)
{
    work();
    pthread_cond_broadcast(&cond_nonfull);
	pthread_detach(pthread_self());
    return NULL;
}


void printQueue(){
    queue<request> temp = pool;
    while(!temp.empty()){
        request req = temp.front();
        temp.pop();
        printf("Requested path: %s, for socket: %i\n", req.path, req.client_socket);
    }
}

int main(int argc, char **argv)
{

	pthread_mutex_init(&mtx, 0);
	pthread_mutex_init(&socket_use, 0);
	pthread_cond_init(&cond_nonempty, 0);
	pthread_cond_init(&cond_nonfull, 0);

    //check input
    if(!readArguments(argc, argv)){
        fprintf(stderr, "Usage: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n");
        exit(1);
    }
    printf("Server parameters ara\n");
    printf("port: %i\n", port);
    printf("thread_pool_size: %i\n", thread_pool_size);
    printf("queue_size: %i\n", queue_size);
    printf("block_size: %i\n", block_size);
    printf("Server was initialized succesfully...\n");
    int sock, newsock;
    unsigned int serverlen, clientlen;	/* Server - client variables */
	struct sockaddr_in server, client;
	struct sockaddr *serverptr, *clientptr;
	struct hostent *rem;

    buffer = (char*) malloc(block_size);
    const int enable = 1;

    clientptr = (struct sockaddr *) &client;
    clientlen = sizeof(client);

    //create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(-1);
	}
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)perror("setsockopt(SO_REUSEADDR) failed");

    //set up server
    server.sin_family = AF_INET;	/* Internet domain */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);	/* The given port */
	serverptr = (struct sockaddr *) &server;
	serverlen = sizeof(server);

    //bind and listen
    if (bind(sock, serverptr, serverlen) < 0){
		perror("bind");	exit(-1);
	}
	if (listen(sock, 5) < 0){
		perror("listen");exit(-1);
	}

    pthread_t *workers = (pthread_t *) malloc(thread_pool_size * sizeof(pthread_t));
    for(int i = 0; i < thread_pool_size; i++)
	    pthread_create(workers+i, 0, worker, 0);
    
    pthread_t con;
    printf("Listening for connections on port:%i \n", port);
    while(1){

        //accept a request and get client info
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0){
            perror("accept"); exit(-1);
        }
        if (setsockopt(newsock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)perror("setsockopt(SO_REUSEADDR) failed");

        if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
            printf("address family: %d\n", client.sin_family);
            printf("AF_INET: %d\n", AF_INET);
            perror("gethostbyaddr");exit(-1);
        }

        printf("Accepted connection from %s\n\n", rem->h_name);

        //call communicator
	    pthread_create(&con, 0, communicator, &newsock);
        pthread_join(con, 0);
        // close(newsock);

    }

	pthread_cond_destroy(&cond_nonempty);
	pthread_cond_destroy(&cond_nonfull);
	pthread_mutex_destroy(&mtx);
	pthread_mutex_destroy(&socket_use);

    return 0;
}
int getDirectoryRequest(char buffer[], int max, int sock){
    while(read(sock, buffer, 1) == 1){
        if(*buffer == '\0')return 1;
        buffer++;
    }
    return 0;
}

int communicate(int newsock, char buffer[]){
    FILE * pipe_fp;
    char command[100];
    char path[MAX_PATH_LEN];

    //get the folder he wants
    getDirectoryRequest(buffer, MAX_PATH_LEN, newsock);
    printf("[Thread: %u]: About to scan folder %s\n", (uint) pthread_self(), buffer);

    //inform him about block size
    if(write(newsock,(char*) &block_size, sizeof(int)) < 0)
        errexit((char*) "writing block size");

    //open pipe with command
    snprintf(command, BUFSIZ, "find %s -type f", buffer);
    if ((pipe_fp = popen(command, "r")) == NULL ){
        perror("popen");exit(1);
    }

    request req;
    req.client_socket = newsock;

    //send number of files
    int files =  0;
    while(fgets(buffer, block_size, pipe_fp) != NULL)files++;
    snprintf(buffer, sizeof(int), "%i", files);
    if(write(newsock, buffer, MAX_PATH_LEN) < 0)errexit((char*) "error sending file number");
    
    pclose(pipe_fp);

    if ((pipe_fp = popen(command, "r")) == NULL ){
        perror("popen");exit(1);
    }

    //send files
    while( fgets(buffer,block_size,pipe_fp) != NULL){
        buffer[strlen(buffer)-1] = '\0';
        memcpy(path, buffer, strlen(buffer)+1);
        memcpy(req.path, buffer, MAX_PATH_LEN);
        printf("[Thread: %u]: Adding file %s to queue\n", (uint) pthread_self(), req.path);
        pool.push(req);
    }

    pclose(pipe_fp);
    return 0;
}

int work(char* path, int newsock, char buffer[]){
    

    printf("[Thread: %u]: Received task <%s,%i>\n",(uint) pthread_self(), path, newsock);
    FILE* fp = fopen(path, "rb");
    if(fp == NULL)errexit(
        (char*) "opening file (worker)"
    );

    struct stat s;
    stat(path, &s);
    int size = (int) s.st_size;

    memcpy(buffer, path, MAX_PATH_LEN);
    snprintf(buffer+MAX_PATH_LEN, 10, "%i", size);
    
    //send file info
    if(write(newsock, buffer, block_size) < 0)errexit(
        (char*) "error writing path to client"
    );

    //send file data
    printf("[Thread: %u]: About to read file %s\n",(uint) pthread_self(), path);
    int n;
    while((n = fread(buffer, 1, block_size, fp)) > 0){
        if(write(newsock, buffer, n) < 0)errexit(
            (char*) "writing error"
        );
        // printf("Server sent %i bytes over socket %i\n", n, newsock);
    }

    fclose(fp);
    return 0;
}

int readArguments(int argc, char** argv){

    int pc, sc, qc, bc;
    pc = sc = qc = bc = 0;

    if(argc != 9)return 0;
    for(int i=1; i < argc; i+=2){
        if(!strcmp(argv[i], "-p")){
            port = atoi(argv[i+1]);
            pc++;
        }
        else if(!strcmp(argv[i], "-s")){
            thread_pool_size = atoi(argv[i+1]);
            sc++;
        }
        else if(!strcmp(argv[i], "-q")){
            queue_size = atoi(argv[i+1]);
            qc++;
        }
        else if(!strcmp(argv[i], "-b")){
            block_size = atoi(argv[i+1]);
            bc++;
        }
        else return 0;
    }

    if(pc != 1 || sc != 1 || qc != 1 || bc != 1)return 0;
    return 1;
}

void printArguments(){
    printf("port: %i\n", port);
    printf("thread_pool_size: %i\n", thread_pool_size);
    printf("queue_size: %i\n", queue_size);
    printf("block_size: %i\n", block_size);
}

