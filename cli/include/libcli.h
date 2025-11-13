#ifndef __lib_cli
#define __lib_cli

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#define	LISTENQ	1024
#define	MAXLINE	4096
#define	SERV_PORT 9877

// rem_money is your money BEFORE you bid this time
int Bid(int, int, int, int);
int Close(int);
// returns sockfd if success
int Connect(const char *);
// send room choice, PIN must be -1 if room is public
int Join(int, int, const char*, int);
// only Player[0] can Lock
int Lock(int);
// returns -1 if write error. use Recv() after
int PlayCard(int, int, int, int);
// only Player[0] can set to Private; PIN must be 4-digit number
int Privt(int, int);
// returns recv string len. recvline must have size >= MAXLINE
int Recv(int, char*);
void Write(int, const void*, size_t);

void err_msg(const char*, ...);
void err_quit(const char*, ...);
void err_sys(const char*, ...);

#ifdef __cplusplus
}
#endif

#endif
