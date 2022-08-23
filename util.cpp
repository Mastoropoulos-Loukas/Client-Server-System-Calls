#include "util.h"
#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;


void addDump(char *path, char* dump_path){
    int path_len = strlen(path);
    int dumb_len = strlen(dump_path);

    if(path_len + dumb_len > MAX_PATH_LEN)errexit(
        (char*) "dumb path too big"
    );
    //shift file path
    memcpy(path+dumb_len, path, path_len+1);

    //add dumb path prefix
    for(int j = 0; j < dumb_len; j++)
        path[j] = dump_path[j];

}


void writeMessage(char* data, int datalen, char* path, char* buffer){
    int pathlen = strlen(path);
    memcpy(buffer, path, pathlen);
    buffer[pathlen] = '\n';
    memcpy(buffer+pathlen+1, data, datalen);
}

int getDirectory(char* path, char dir[], char fn[]){

    int len = strlen(path);
    int foundFS = 0;
    int last_FS_pos = len;
    memcpy(dir, "dest/", strlen("dest/"));
    dir += strlen("dest/");

    for(int i = 0; i < len; i++){
        if(path[i] == '/'){
            if(!foundFS) foundFS = 1;
            last_FS_pos = i;
        }
        dir[i] = path[i];
    }
    if(foundFS)dir[last_FS_pos] = '\0';

    int left = len - last_FS_pos;
    if(left)memcpy(fn, path+last_FS_pos+1, left);
    fn[left] = '\0';

    return last_FS_pos;
}

void readMessage(char *buffer, int bufferlen, char **path, char** data){
    *path = buffer;
    int dataStart;
    for(dataStart = 0; buffer[dataStart] != '\n'; dataStart++);
    buffer[dataStart] = '\0';
    *data = buffer+dataStart+1;
}

void errexit(char* msg){
    perror(msg);
    exit(1);
}

int write_all(int fd, char *buff, size_t size) {
    int sent, n;
    for(sent = 0; sent < size; sent+=n) {
        if ((n = write(fd, buff+sent, size-sent)) == -1) 
            return -1; /* error */
    }
    return sent;
}

int read_all(int fd, char*buff, size_t size){
    int total, n;
    for(total = 0; total < size; total+=n){
        if ((n = read(fd, buff+total, size - total)) == -1)
            return -1;
        else if( n == 0)return total;
    }
    return total;
}
