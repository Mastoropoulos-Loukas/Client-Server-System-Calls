#ifndef UTIL_H
#define UTIL_H

#define MAX_PATH_LEN 150

#include <stdio.h>

void errexit(char *msg);
void writeMessage(char* data, int datalen, char* path, char* buffer);
void readMessage(char *buffer, int bufferlen, char** path, char** data);
int write_all(int fd, char *buff, size_t size);
int read_all(int fd, char *buff, size_t size);
int getDirectory(char *path, char dir[], char fn[]);
void addDump(char *path, char* dump_path);

#endif