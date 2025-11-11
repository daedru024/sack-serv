#ifndef __lib_cli
#define __lib_cli

#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <sys/filio.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	LISTENQ	1024
#define	MAXLINE	4096
#define	SERV_PORT 9877
//functions
// connect

// close
// play_card
// bid
// recv_bid
// send_room
// recv_msg
#endif