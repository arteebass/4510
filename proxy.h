/*
 *          Filename: proxy.h
 *  Last Modified on: Apr 11, 2016
 *        Developers: Rueben Tiow, Hunter Garrett
 *       Description: Prototype for simple proxy server
 */
using namespace std;
#ifndef PROXY_H_
#define PROXY_H_

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <iostream>
#include <queue>
#include <semaphore.h>
#include <arpa/inet.h>
#include <map>

#define CRLF "\r\n"
#define MSG_BUF_SIZE	1000000
#define MAX_THREADS		30
#define SEM_PERMISSIONS	0644

int server_s;
const char* errorMessage = "HTTP/1.0 500 'Internal Server Error'\r\n\r\n";
static bool done = 0;
std::queue<int>* socketQ;
pthread_t* pool;
pthread_mutex_t* count_mutex;
sem_t* job_queue_count;
char SEM_NAME[] = "sem";

void error(const char *err) {
	perror(err);
	exit(EXIT_FAILURE);
}

void receiveMsg(int s, int size, char *ptr, bool headerOnly);
void sendMsg(int s, int size, char *ptr);
void closeSocket(int sock, char* &Buffer);
void* parse(void* threads);
void initSynchronization();
void initThreadPool();
void setupSigHandlers();
void termination_handler(int signum);
#endif
