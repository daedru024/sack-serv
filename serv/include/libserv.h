#ifndef __lib_serv
#define __lib_serv

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>

#define	LISTENQ	1024
#define	MAXLINE	4096
#define	SERV_PORT 9877

int Accept(int, struct sockaddr*, socklen_t*);
int Close(int);
void Listen(int, int);
int Poll(struct pollfd*, unsigned long);
void Write(int, const void*, size_t);

void err_msg(const char*, ...);
void err_quit(const char*, ...);
void err_sys(const char*, ...);
#endif