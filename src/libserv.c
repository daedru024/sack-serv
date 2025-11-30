#include "libserv.h"

#define DEBUG

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map
extern time_t lst_conn[FOPEN_MAX]; //last msg timestamp
extern Queue* q;

/**** Game mechanism ****/

void ExitCli(int idx, Rooms* Rm, int rID) {
    int pID = 0;
    if(Rm->stat == 0 || (Rm->stat == 4 && Rm->rnd == 0)) {
        // resend room info, unlock
        char tmp[MAXLINE];
        Rm->stat = 0;
        if(--(Rm->num_players) == 0) {
            init_RoomInfo(Rm);
            return;
        }
        for(int i=0; i<Rm->num_players; i++) {
            if(Rm->plyData[i].i == idx) {
                Rm->plyData[i] = Rm->plyData[i+1];
                Rm->plyData[i+1].i = idx;
            }
        }
        GetOneRoomInfo(Rm, rID, tmp);
        SendAll(Rm, tmp, 1);
        return;
    }
    char tmp[MAXLINE];
    for(int i=0; i<Rm->num_players; i++) {
        if(Rm->plyData[i].i == idx) {
            pID = i;
            break;
        }
    }
    if(Rm->stat == 1 || Rm->stat == 2) {
        if(Rm->auto_player) {
            char tmp[MAXLINE];
            sprintf(tmp, "ap %d\n", pID);
            SendAll(Rm, tmp, 0);
            for(int i=0; i<Rm->num_players; i++) 
                Close(clients[Rm->plyData[i].i].fd);
            init_RoomInfo(Rm);
            return;
        }
        else {
            sprintf(tmp, "ap %d\n", pID);
            SendAll(Rm, tmp, 0);

            //TODO auto play mech
            return;
        }
    }
    if(Rm->stat == 3) return;
}

bool isValidStr(char* tar, int n) {
    tar[n] = 0;
    if(n > 1 && tar[0] == '1' && tar[1] == '1') {
        //11 {RoomID} {username} {PIN}
        if(!isdigit(tar[3])) return 0;
        for(int i=n-5; i<n; i++) 
            if(!isdigit(tar[i]) && tar[i] != ' ') return 0;
        return 1;
    }
    for(int i=0; i<n; i++) 
        if(!isdigit(tar[i]) && tar[i] != ' ') return 0;
    return 1;
}

void MakePlay(Rooms* tar, int pID) {
    //TODO
}

void RecvBid(Rooms* tar, int pID, char* msg) {
    //TODO
}

void RecvPlay(Rooms* tar, int pID, char* msg) {
    //TODO
}

void StartGame(Rooms* Rm) {
    char buf[MAXLINE];
    for(int i=0; i<Rm->num_players; i++) {
        int idx = Rm->plyData[i].i;
        int n;
        if(clients[idx].revents & (POLLRDNORM | POLLERR)) {
            if((n = read(clients[idx].fd, buf, MAXLINE)) <= 0) {
                if(n == 0 || errno == ECONNRESET) {
                    Delete(q, idx);
                    ExitCli(idx, Rm, in_room[idx]);
                    in_room[idx] = -1;
                    Close(clients[idx].fd);
                    clients[idx].fd = -1;
                    return;
                }
                else err_sys("read error");
            }
        }
    }
    sprintf(buf, "GAMESTART\n");
    SendAll(Rm, buf, 0);
    if(Rm->stat == 4) Rm->stat = 1;
    return;
}

void bitw1(int* tar, int k) {
    *tar = ((*tar) | (1<<k));
    return;
}

bool bitis1(int tar, int k) {
    return (tar&(1<<k));
}
