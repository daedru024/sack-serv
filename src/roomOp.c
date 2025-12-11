#include "libserv.h"

/**** Room Operations ****/

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map
extern const int AbdMoney[3][5];

void ChooseColor(Rooms* Rm, int idx, char* msg) {
    int col, k;
    char buf[MAXLINE];
    sscanf(msg, "7 %d", &col);
    for(int i=0; i<Rm->num_players; i++) {
        if(Rm->plyData[i].col == col) {
            GetOneRoomInfo(Rm, in_room[idx], buf);
            SendAll(Rm, buf, 1);
            return;
        }
        else if(Rm->plyData[i].i == idx) k = i;
    }
    Rm->plyData[k].col = col;
#ifdef DEBUG
    printf("Chose color %d for %d\n", col, k);
#endif
    GetOneRoomInfo(Rm, in_room[idx], buf);
    SendAll(Rm, buf, 1);
}

void CloseRoom(Rooms* tar) {
    for(int j=0; j<tar->num_players; j++) 
        Close(tar->plyData[j].i);
    init_RoomInfo(tar);
    return;
}

void GetOneRoomInfo(Rooms* tar, int rID, char* ret) {
    //in {RoomID} {n_Players} {username[:] color[:]} {locked} {PIN} {playerID}
    char tmp[20];
    sprintf(ret, "in %d %d ", rID, tar->num_players);
    for(int i=0; i<tar->num_players; i++) {
        sprintf(tmp, "%s %d ", tar->plyData[i].username, tar->plyData[i].col);
        strcat(ret, tmp);
    }
    sprintf(ret, "%s%d %04d ", ret, (tar->stat==4 && tar->rnd==0), tar->passkey);
    return;
}

void GetRoomInfo(Rooms* tar, int rID, char* ret) {
    if(tar->stat == 0) {
        // available
        sprintf(ret, "ra %d %d ", rID, tar->num_players);
        for(int i=0; i<tar->num_players; i++) {
            sprintf(ret, "%s%s %d ", ret, tar->plyData[i].username, tar->plyData[i].col);
        }
        if(tar->passkey == 10000) sprintf(ret, "%s0 ", ret);
        else sprintf(ret, "%s1 ", ret);
    }
    else // unavailable
        sprintf(ret, "ru %d %d %d ", rID, tar->num_players, tar->rnd);
    return;
}

void GetScore(Rooms* tar) {
    char buf[MAXLINE];
    char tmp[100];
    sprintf(buf, "ws ");
    //ws {won[:] values[:]} {score[:]}
    for(int k=0; k<9; k++) {
        sprintf(tmp, "%d %d ", tar->wonstk[k], tar->values[k]);
        strcat(buf, tmp);
    }
    for(int k=0; k<tar->num_players; k++) {
        tar->plyData[k].score += tar->plyData[k].rem_money;
        sprintf(tmp, "%d ", tar->plyData[k].score);
        strcat(buf, tmp);
    }
    SendAll(tar, buf, 0);
    CloseRoom(tar);
}

void init_PlayerInfo(Rooms* tar, int i) {
    bzero(&(tar->plyData[i]), sizeof(Player));
    tar->plyData[i].col = -1;
    tar->plyData[i].rem_money = 15;
    tar->plyData[i].LastBid = 0;
    tar->plyData[i].score = 0;
    tar->plyData[i].i = -1;
}

void init_RoomInfo(Rooms* tar) {
    tar->stat = 0;
    tar->passkey = 10000;
    tar->num_players = 0;
    tar->rnd = 0;
    memset(tar->stks, -1, 45*sizeof(int));
    memset(tar->values, 0, 9*sizeof(int));
    memset(tar->rabbit, -1, 5*sizeof(int));
    memset(tar->wonstk, -1, 9*sizeof(int));
    memset(tar->abdMoney, 0, 5*sizeof(int));
    tar->sPlayer = 0;
    tar->nPlayer = 0;
    tar->lstbid = 0;
    tar->aban = 0;
    tar->auto_player = 0;
    for(int i=0; i<5; i++) 
        init_PlayerInfo(tar, i);
    return;
}

void JoinRoom(Rooms* tar, char* usrn, int idx) {
    int playerID = tar->num_players;
    strcpy(tar->plyData[playerID].username, usrn);
    tar->plyData[playerID].i = idx;
    if(++tar->num_players == 5) tar->stat = 4;
    return;
}

void Lock(Rooms* tar, int i) {
    if(tar->plyData[0].i == i && tar->num_players > 2 && tar->stat == 0)
        tar->stat = 4;
#ifdef DEBUG
    if(tar->stat == 4) printf("Locked room %d\n", in_room[i]);
    else printf("Status %d\n", tar->stat);
#endif
    if(tar->num_players == 3) {
        //add auto player
        tar->num_players++;
        strcpy(tar->plyData[3].username, "BOT");
        tar->plyData[3].i = -1;
        tar->auto_player = 1;
        for(int k=0; k<5; k++) {
            tar->plyData[3].col = k;
            for(int j=0; j<3; j++) {
                if(tar->plyData[j].col == k) {
                    tar->plyData[3].col = -1;
                    break;
                }
            }
            if(tar->plyData[3].col != -1) break;
        }
    }
    char tmp[MAXLINE];
    GetOneRoomInfo(tar, in_room[i], tmp);
    SendAll(tar, tmp, 1);
    for(int j=0; j<tar->num_players; j++) 
        if(tar->plyData[j].col == -1) return;
    StartGame(tar);
}

void MakePrivate(Rooms* tar, int idx, char* Pwd, int k) {
    int PIN;
    sscanf(Pwd, "5 %d", &PIN);
    if(PIN == 10000 && tar->plyData[0].i == idx) 
        tar->passkey = 10000;
    else if(k >= 2 && tar->passkey == 10000) {
        sprintf(Pwd, "re 6\n"); //6 too many private rooms
        Write(clients[idx].fd, Pwd, strlen(Pwd));
        return;
    }
    else if(tar->plyData[0].i == idx && k < 2) {
        tar->passkey = PIN;
    }
    if(tar->passkey != 10000) tar->madePriv = time(NULL);
    char tmp[MAXLINE];  
    GetOneRoomInfo(tar, in_room[idx], tmp);
    SendAll(tar, tmp, 1);
}

void StartGame(Rooms* Rm) {
    char buf[MAXLINE];
    sprintf(buf, "GAMESTART\n");
    SendAll(Rm, buf, 0);
    if(Rm->stat == 4) {
        Rm->stat = 1;
        memcpy(Rm->abdMoney, AbdMoney[Rm->num_players-3], sizeof AbdMoney[Rm->num_players-3]);
    }
    return;
}

void Unlock(Rooms* tar, int i) {
    if(tar->stat == 4 && tar->plyData[0].i == i)
        tar->stat = 0;
    char tmp[MAXLINE];  
    GetOneRoomInfo(tar, in_room[i], tmp);
    SendAll(tar, tmp, 1);
}
