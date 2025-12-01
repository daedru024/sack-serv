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
    int i; //-1 if auto play
    int col; //-1 if not ready
    int rem_money;
    int LastBid; //-1 if aborted
    int MASK_Uc;
    int MASK_St;
    int score;
} Player;

typedef struct {
    int stat; //0 vacant 1 play_card 2 bid 3 score 4 otherwise
    int passkey; //10000 if public
    int num_players;
    int rnd; //0-9
    int stks[9][5]; //round i card j
    int values[9]; 
    int rabbit[5];  //player x’s rabbit is rabbit[x], x=0-4
    int wonstk[9]; //-1 if nobody won
    int sPlayer; //who plays first
    int nPlayer; //who plays now
    int lstbid; //last bidding value
    int aban; //number of players who abandoned bid
    int abdMoney[5]; //amount of money player gets after abandoning bid
    bool auto_player;
    Player plyData[5];
} Rooms;

typedef struct Node {
    int sockfd, roomID, i;
    struct Node* nxt;
} Node;

typedef struct {
    Node* dm_head;
    Node* tail;
} Queue;

//queue func

void init_Queue(Queue*);
void Delete(Queue*, int);
bool isEmpty(Queue*);
void pop(Queue*);
void push(Queue*, int, int, int);
Node* front(Queue*);

//err func

void err_msg(const char*, ...);
void err_quit(const char*, ...);
void err_sys(const char*, ...);


//elem func

int Accept(int, struct sockaddr*, socklen_t*);
// Deletes node, changes in_room and clients[i].fd
int Close(int);
int Closefd(int);
void Listen(int, int);
int Poll(struct pollfd*, unsigned long);
void SendAll(Rooms*, char*, int);
int Write(int, const void*, size_t);


//game mechanism

void AutoBid(Rooms*, int);
void AutoPlay(Rooms*, int);
void ChooseColor(Rooms*, int, char*);
void ExitCli(int, Rooms*, int, int);
void GetOneRoomInfo(Rooms*, int, char*);
void GetRoomInfo(Rooms*, int, char*);
void GetScore(Rooms*);
void init_PlayerInfo(Rooms*, int);
void init_RoomInfo(Rooms*);
bool isValidStr(char*, int);
void JoinRoom(Rooms*, char*, int);
void Lock(Rooms*, int);
void MakeBid(Rooms*);
void MakePrivate(Rooms*, int, char*, int);
void Rabbit(Rooms*, int, char*);
void RecvBid(Rooms*, char*);
void RecvPlay(Rooms*, char*);
void StartGame(Rooms*);
void Unlock(Rooms*, int);

void bitw1(int*, int);
bool bitis1(int, int);

#endif
