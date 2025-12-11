#include "libserv.h"

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map
extern time_t lst_conn[FOPEN_MAX]; //last msg timestamp
extern Queue* q;
extern int maxi;

void ApConnect(Rooms* tar, int pID, int rID) {
    int sockfd;
    struct sockaddr_in servaddr;
    char servip[10] = "127.0.0.1";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) 
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT+1);
    
    if(inet_pton(AF_INET, servip, &servaddr.sin_addr) <= 0) 
        err_sys("inet_pton error");
    if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) 
        err_sys("connect error");
    int idx = tar->plyData[pID].i;
    if(idx != -1) {
        clients[idx].fd = sockfd;
    }
    else {
        for(int k=1; k<FOPEN_MAX; k++) {
            if(clients[k].fd == -1) {
                clients[k].fd = sockfd;
                tar->plyData[pID].i = k;
                idx = k;
                if(k>maxi) maxi = k;
                break;
            }
        }
    }
    if(idx != -1) {
        clients[idx].events = POLLRDNORM;
        in_room[idx] = rID;
        lst_conn[idx] = time(NULL);
        push(q, sockfd, rID, idx);
    }
    //send info
    char buf[MAXLINE];
    //{playerID} {num_players} {MASK_Uc} {rem_money} {sPlayer} {aban}
    sprintf(buf, "%d %d %d %d %d %d\n", pID, tar->num_players, tar->plyData[pID].MASK_Uc, tar->plyData[pID].rem_money, tar->sPlayer, tar->aban);
    Write(sockfd, buf, strlen(buf));
    if(strlen(tar->LastBroadcast) > 0) Write(sockfd, tar->LastBroadcast, strlen(tar->LastBroadcast));
}

void ExitCli(int idx, Rooms* Rm, int rID, int pID) {
    if(rID == -1) {
        Close(idx);
        return;
    }
    char tmp[MAXLINE];
    if(Rm->stat == 0 || (Rm->stat == 4 && Rm->rnd == 0)) {
        // resend room info, unlock
        Rm->stat = 0;
        if(--Rm->num_players == 0) {
            init_RoomInfo(Rm);
            Close(idx);
            return;
        }
        for(int i=0; i<Rm->num_players; i++) {
            if(Rm->plyData[i].i == idx) {
                Rm->plyData[i] = Rm->plyData[i+1];
                Rm->plyData[i+1].i = idx;
            }
        }
        GetOneRoomInfo(Rm, rID, tmp);
        Close(idx);
        init_PlayerInfo(Rm, Rm->num_players);
        SendAll(Rm, tmp, 3);
        return;
    }
    if(pID == -1) {
        for(int i=0; i<Rm->num_players; i++) {
            if(Rm->plyData[i].i == idx) {
                Close(idx);
                Rm->plyData[i].i = -1;
                pID = i;
                break;
            }
        }
    }
    if(Rm->stat == 1 || Rm->stat == 2) {
        sprintf(tmp, "ap %d\n", pID);
        int n = 0;
        SendAll(Rm, tmp, 2);
        for(int i=0; i<Rm->num_players; i++) if(Rm->plyData[i].i == -1) n++;
        if(Rm->rnd == 0 || n > 1 || Rm->auto_player) {
            CloseRoom(Rm);
            return;
        }
        else {
            Rm->auto_player = 1;
            ApConnect(Rm, pID, rID);
        }
    }
    if(Rm->stat == 3) {
        for(int i=0; i<Rm->num_players; i++) {
            if(Rm->plyData[i].i == idx) {
                Rm->plyData[i].i = -1;
                break;
            }
        }
        Close(idx);
        for(int i=0; i<Rm->num_players; i++) 
            if(Rm->plyData[i].i != -1) return;
        init_RoomInfo(Rm);
#ifdef DEBUG
        printf("Room %d reset to empty.\n", rID);
#endif
    }
}

bool isValidStr(char* tar, int n) {
    tar[n] = 0;
    if(n > 1 && tar[0] == '1' && tar[1] == '1') {
        //11 {RoomID} {username} {PIN}
        if(!isdigit(tar[3])) return 0;
        for(int i=n-5; i<n; i++) 
            if(!isdigit(tar[i]) && tar[i] != ' ' && tar[i] != '\n') return 0;
        return 1;
    }
    for(int i=0; i<n; i++) 
        if(!isdigit(tar[i]) && tar[i] != ' ' && tar[i] != '\n') return 0;
    return 1;
}

void SendAll(Rooms* tar, char* msg, int c) {
    //c: 1 print id, 2 from exitcli, 3 from exitcli and print id, 10 if ignore player 0
    char tmp[MAXLINE];
    int k = (c == 10);
    for(int i=k; i<tar->num_players; i++) {
        if(tar->plyData[i].i == -1) continue;
        if(c == 1 || c == 3)
            sprintf(tmp, "%s %d \n", msg, i);
        else {
            strcpy(tmp, msg);
            if(tmp[0] != 'a') strcpy(tar->LastBroadcast, tmp);
        }
        if(Write(clients[tar->plyData[i].i].fd, tmp, strlen(tmp)) == -1) {
            if(c != 2) {
                ExitCli(tar->plyData[i].i, tar, in_room[tar->plyData[i].i], i);
                if(c == 3) return;
                continue;
            }
            Close(tar->plyData[i].i);
            tar->plyData[i].i = -1;
            continue;
        }
        char tmp[3];
        lst_conn[tar->plyData[i].i] = time(NULL);
        push(q, clients[tar->plyData[i].i].fd, in_room[tar->plyData[i].i], tar->plyData[i].i);
    }
    return;
}

void bitw1(int* tar, int k) {
    *tar = ((*tar) | (1<<k));
    return;
}

bool bitis1(int tar, int k) {
    return (tar&(1<<k));
}
