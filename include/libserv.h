#ifndef __lib_serv
#define __lib_serv

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>

#define	LISTENQ	1024
#define	MAXLINE	4096
#define	SERV_PORT 9877


//data struct

typedef struct {
    char username[10];
    int sockfd; //-1 if auto play
    int col; //-1 if not ready
    int rem_money;
    int MASK_Uc;
    int MASK_St;
} Player;

typedef struct {
    int stat; //0 vacant 1 play_card 2 bid 3 score 4 otherwise
    int passkey; //10000 if public
    int num_players;
    int rnd; //0-9
    int stks[9][5]; //round i card j
    int rabbit[5];  //player x’s rabbit is rabbit[x], x=0-4
    int wonstk[9]; //-1 if nobody won
    int sPlayer; //who plays first
    bool auto_player;
    Player plyData[5];
} Rooms;


//elem func

int Accept(int, struct sockaddr*, socklen_t*);
int Close(int);
void Listen(int, int);
int Poll(struct pollfd*, unsigned long);
void SendAll(Rooms*, char*);
int Write(int, const void*, size_t);


//err func

void err_msg(const char*, ...);
void err_quit(const char*, ...);
void err_sys(const char*, ...);


//game mechanism

void ExitCli(int, Rooms*, int);
void GetRoomInfo(Rooms*, int, char*);
void init_RoomInfo(Rooms*);
bool isValidStr(char*, int);
void JoinRoom(Rooms*, char*, int);
void MakePlay(Rooms*, int);
void MakePrivate(Rooms*, char*);
void RecvBid(Rooms*, int, char*);
void RecvPlay(Rooms*, int, char*);
//random rabbit

void bitw1(int*, int);
bool bitis1(int, int);

#endif
