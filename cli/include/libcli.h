#ifndef __lib_cli
#define __lib_cli

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
//#include <sys/filio.h>

#include <netinet/in.h>
#include <arpa/inet.h>

//#include <sys/time.h>
//#include <time.h>
//#include <errno.h>
//#include <fcntl.h>
//#include <netdb.h>
//#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	LISTENQ	1024
#define	MAXLINE	4096
#define	SERV_PORT 9877

//functions
// connect, return sockfd if success
int Connect(const char *);
int Close(int);
// returns -1 if write error. use Recv() after
int PlayCard(int, int, int, int);
// bid, rem_money is your money BEFORE you bid this time
int Bid(int, int, int, int);
// send room choice, PIN must be -1 by default
int Join(int, int, const char*, int);
// returns recv string len. recvline must have size >= MAXLINE
int Recv(int, char*);
// only Player[0] can Lock
int Lock(int);
// only Player[0] can set to Private; PIN must be 4-digit number
int Privt(int, int);

#ifdef __cplusplus
}
#endif

#endif
