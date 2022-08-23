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
#include <string.h>
#include <fcntl.h>
#include "util.h"

using namespace std;

int port;
in_addr server_ip;
char* directory;

int readArguments(int argc, char** argv);
void printArguments();
int writeFile(int sock, char buff[], int block_size, FILE* fd, int size);
int readFile(int sock, char buffer[], char* path, char dir[], char filename[], char command[], int block_size);

int main(int argc, char **argv)
{
    int sock;
	unsigned int serverlen;
	struct sockaddr_in server;
	struct sockaddr *serverptr;
	struct hostent *rem;
    const int enable = 1;

    //check input
    if(!readArguments(argc, argv)){
        fprintf(stderr, "Usage: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
        exit(1);
    }
    printf("Client parameters are:\n");
    printf("server IP: %s\n", inet_ntoa(server_ip));
    printf("port: %i\n", port);
    printf("directory: %s\n", directory);
    printf("Connecting to %s on port %i\n", inet_ntoa(server_ip), port);

    //make socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(-1);
	}
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)perror("setsockopt(SO_REUSEADDR) failed");

    //get server
    if ((rem = gethostbyaddr(&server_ip, sizeof(server_ip), AF_INET)) == NULL){	
		perror("gethostbyname");
		exit(-1);
	}

    //set server
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);
    serverptr = (struct sockaddr *) &server;
    serverlen = sizeof(server);

    //connect to server
    if (connect(sock, serverptr, serverlen) < 0){
		  perror("connect");exit(-1);
	  }
    
    // printf("Client using socket %i \n", sock);

    //write folder request
    if(write(sock, directory, strlen(directory)+1) == -1){
        perror("write directory");
        exit(1);
    }

    //get block size
    int n, block_size, files;
    if((n = read(sock, (char*) &block_size, sizeof(int))) < 0)
        errexit((char*)"reading block size");
    char buffer[block_size];

    //get number of files
    if((n = read(sock, buffer, MAX_PATH_LEN) < 0))errexit((char*)"error reading files number");
    files = atoi(buffer);
    // printf("%i files left\n", files);

    char *path;
    char dir[MAX_PATH_LEN], filename[MAX_PATH_LEN], command[200]; 

    while(files > 0){
        files --;
        readFile(sock, buffer, path, dir, filename, command, block_size); if(files == 0)break;
        // printf("\n%i files left\n", files); 
        }

    close(sock);
    exit(0);
    
    return 0;
}
int readFile(int sock, char buffer[], char* path, char dir[], char filename[], char command[], int block_size){

    int n;

     //get file info
    if((n = read(sock, buffer, block_size)) < 0)
        errexit((char*)"read_all file");
    if(n == 0){
        // printf("no more files to read\n");
        return 1;
    }
    path = buffer;
    int size = atoi(buffer+MAX_PATH_LEN);
    // printf("Client got path: %s\nClient got file size: %i\n", path, size);

    //create folder for file
    getDirectory(path, dir, filename);
    // printf("Client creating dir: %s\n", dir);
    snprintf(command, 200, "mkdir -p %s", dir);
    system(command);

    //open file stream
    addDump(path, (char*) "dest/");
    FILE* fd = fopen(path, "wb");
    if(fd == NULL)errexit(
        (char*) "opening file error"
    );

    //write file data
    char temp[MAX_PATH_LEN];
    memcpy(temp, path, MAX_PATH_LEN);
    // printf("%s\n", path);
    writeFile(sock, buffer, block_size, fd, size);
    printf("Received: %s\n", temp);
    fclose(fd);
    return 0;
}

int writeFile(int sock, char buffer[], int block_size, FILE* fd, int size){
    int n;
    int total = 0;
    int left = size;

    //read and write file data
    while((n = read(
        sock, 
        buffer,
        block_size < size - total ? block_size : size - total
    )) > 0){
        if(fwrite(buffer, 1, n, fd) == -1)errexit(
            (char*) "writing error"
        );
        // printf("read %i bytes\n", n);
        total += n;
        if(total == size) return 0;
        if(total > size)errexit(
            (char*) "somehow read more than i should"
        );
    }
    if(n < 0)errexit(
        (char*) "error with reading file"
    );

    return 0;
}

int readArguments(int argc, char** argv){

    int pc, ic, dc;
    pc = ic = dc = 0;

    if(argc != 7)return 0;
    for(int i=1; i < argc; i+=2){
        if(!strcmp(argv[i], "-p")){
            port = atoi(argv[i+1]);
            pc++;
        }
        else if(!strcmp(argv[i], "-i")){
            inet_aton(argv[i+1], &server_ip);
            ic++;
        }
        else if(!strcmp(argv[i], "-d")){
            directory = argv[i+1];
            dc++;
        }
        else return 0;
    }

    if(pc != 1 || ic != 1 || dc != 1)return 0;
    return 1;
}

void printArguments(){
    printf("port: %i\n", port);
    printf("server_ip: %s\n", inet_ntoa(server_ip));
    printf("directory: %s\n", directory);
}
