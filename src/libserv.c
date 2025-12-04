#include "libserv.h"

#define DEBUG

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map
extern time_t lst_conn[FOPEN_MAX]; //last msg timestamp
extern Queue* q;
extern const int AbdMoney[3][5];
extern const int Cards[10];

/**** Game mechanism ****/

void AutoBid(Rooms* tar, int pID) {
    char buf[MAXLINE];
    sprintf(buf, "17 %d 0 %d", pID, tar->plyData[pID].rem_money);
#ifdef DEBUG
    printf("AutoBid\n");
#endif
    RecvBid(tar, buf);
}

void AutoPlay(Rooms* tar, int pID) {
    if(tar->stks[tar->rnd-1][pID] != -1) return;
    char buf[MAXLINE];
    int r = rand() % (10-tar->rnd);
    for(int i=0; i<10; i++) {
        if(bitis1(tar->plyData[pID].MASK_Uc, i)) continue;
        if(--r == 0) {
            sprintf(buf, "13 %d %d %d", pID, i, tar->plyData[pID].MASK_Uc);
            RecvPlay(tar, buf);
            return;
        }
    }
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
        if(--(Rm->num_players) == 0) {
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
        if(n > 1) {
            for(int i=0; i<Rm->num_players; i++) 
                Close(Rm->plyData[i].i);
            init_RoomInfo(Rm);
            return;
        }
        else {
            Rm->auto_player = 1;
            if(Rm->nPlayer == pID) {
                if(Rm->stat == 1) AutoPlay(Rm, pID);
                else AutoBid(Rm, pID);
            }
            return;
        }
    }
    if(Rm->stat == 3) Close(idx);
}

void GetScore(Rooms* tar) {
    //TODO
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

void Rabbit(Rooms* tar, int i, char* msg) {
    //client chooses rabbit
    //19 {rabbit}
    if(msg[0] != '1' || msg[1] != '9') return;
    int pID = 0;
    for(int j=0; j<tar->num_players; j++) {
        if(tar->plyData[j].i == i) {
            pID = j;
            break;
        }
    }
    int r = rand() % 10;
    int c;
    sscanf(msg, "19 %d", &c);
    c = (c+r) % 10;
    int k = (pID+1) % tar->num_players;
    tar->rabbit[k] = c;
    char tmp[10];
    sprintf(tmp, "ri %d\n", c);
    if(Write(clients[tar->plyData[k].i].fd, tmp, strlen(tmp)) == -1) 
        ExitCli(tar->plyData[i].i, tar, in_room[tar->plyData[i].i], i);
    for(int j=0; j<tar->num_players; j++) 
        if(tar->rabbit[j] == -1) return;
    tar->rnd = 1;
}

void RecvBid(Rooms* tar, char* msg) {
    //17 {PlayerID} {amount} {rem_money} 
    if(msg[0] != '1' || msg[1] != '7') return;
    int pID, pri, rem, cd = -1, nply = 0;
    char buf[MAXLINE];
    sscanf(msg, "17 %d %d %d", &pID, &pri, &rem);
    //b {PlayerID} {amount} {nPlayer} {card}
    if(pID != tar->nPlayer || (pri <= tar->lstbid && pri != 0)) nply = -1;
    else if(rem != tar->plyData[pID].rem_money) nply = -1;
    else if(pri == 0) {
        tar->plyData[pID].rem_money += tar->abdMoney[tar->aban++];
        tar->plyData[pID].LastBid = -1;
        cd = tar->stks[tar->rnd-1][(tar->sPlayer+tar->aban)%tar->num_players];
    }
    else {
        tar->lstbid = pri;
        tar->plyData[pID].LastBid = pri;
        tar->wonstk[tar->rnd-1] = pID;
    }
    tar->nPlayer = (tar->nPlayer+1) % tar->num_players;
    while(tar->nPlayer != pID) {
        if(tar->plyData[tar->nPlayer].LastBid != -1) break;
        tar->nPlayer = (tar->nPlayer+1) % tar->num_players;
    }
    nply = (nply == 0) ? tar->nPlayer : nply;
    sprintf(buf, "b %d %d %d %d\n", pID, pri, nply, cd);
    SendAll(tar, buf, 0);
    if(tar->stat == 0) return;
    if(tar->aban >= tar->num_players-1) {
        if(tar->plyData[tar->nPlayer].LastBid == 0) return;
        //end bid
        pri = -1;
        for(int i=0; i<tar->num_players; i++) {
            if(tar->plyData[i].LastBid > 0) {
                tar->plyData[i].rem_money -= tar->plyData[i].LastBid;
                pri = i;
            }
            tar->plyData[i].LastBid = 0;
        }
        //be {PlayerID} {amount} {sPlayer}
        tar->sPlayer = (tar->sPlayer+1) % tar->num_players;
        sprintf(buf, "be %d %d %d\n", pri, tar->lstbid, tar->sPlayer);
        SendAll(tar, buf, 0);
        if(tar->stat == 0) return;
        if(pri >= 0) 
            tar->plyData[pri].score += tar->values[tar->rnd-1];
        tar->lstbid = 0;
        tar->nPlayer = tar->sPlayer;
        tar->aban = 0;
        if(++tar->rnd == 10) {
            tar->stat = 3;
            return;
        }
        tar->stat = 1;
        return;
    }
    if(tar->auto_player && clients[tar->plyData[tar->nPlayer].i].fd == -1) {
        AutoBid(tar, tar->nPlayer);
        return;
    }
}

void RecvPlay(Rooms* tar, char* msg) {
    //13 {PlayerID} {cardID} {MaskUc}
    int pID, cID, mskUc;
    if(msg[0] != '1' || msg[1] != '3') return;
    sscanf(msg, "13 %d %d %d", &pID, &cID, &mskUc);
    int cd;
    char buf[MAXLINE];
    if(pID != tar->nPlayer) cd = 0;
    else if(mskUc != tar->plyData[pID].MASK_Uc || bitis1(mskUc, cID)) 
        cd = -1;
    else {
        bitw1(&(tar->plyData[pID].MASK_Uc), cID);
        cd = 1;
        tar->stks[tar->rnd-1][pID] = cID;
        if(cID >= 8 || (cID == 2 && tar->rabbit[pID] >= 8)) {
            if(cID == 2) cID = tar->rabbit[pID];
            if(tar->values[tar->rnd-1] >= 8 || tar->values[tar->rnd-1] == -1) 
                tar->values[tar->rnd-1] = -1;
            else tar->values[tar->rnd-1] = cID;
        }
    }
    //c {PlayerID} {code}
    sprintf(buf, "c %d %d\n", pID, cd);
    SendAll(tar, buf, 0);
    if(tar->stat == 0) return;
    tar->nPlayer = (tar->nPlayer+1) % tar->num_players;

    if(tar->nPlayer == tar->sPlayer) {
        tar->stat = 2;
        switch(tar->values[tar->rnd-1]) {
        case -1:
            //ignore all dogs
            tar->values[tar->rnd-1] = 0;
            for(int i=0; i<tar->num_players; i++) {
                int j = tar->stks[tar->rnd-1][i];
                if(j == 2) j = tar->rabbit[i];
                if(j >= 8) continue;
                else tar->values[tar->rnd-1] += Cards[j];
            }
            break;
        case 8:
            //DOG
            tar->values[tar->rnd-1] = 0;
            int mx = 0;
            for(int i=0; i<tar->num_players; i++) {
                int j = tar->stks[tar->rnd-1][i];
                if(j == 2) j = tar->rabbit[i];
                tar->values[tar->rnd-1] += Cards[j];
                if(Cards[j] > mx) mx = Cards[j];
            }
            tar->values[tar->rnd-1] -= mx;
            break;
        case 9:
            //dog
            tar->values[tar->rnd-1] = 0;
            int mn = 0;
            for(int i=0; i<tar->num_players; i++) {
                int j = tar->stks[tar->rnd-1][i];
                if(j == 2) j = tar->rabbit[i];
                tar->values[tar->rnd-1] += Cards[j];
                if(Cards[j] < mn) mn = Cards[j];
            }
            tar->values[tar->rnd-1] -= mn;
            break;
        default:
            tar->values[tar->rnd-1] = 0;
            for(int i=0; i<tar->num_players; i++) {
                int j = tar->stks[tar->rnd-1][i];
                if(j == 2) j = tar->rabbit[i];
                tar->values[tar->rnd-1] += Cards[j];
            }
        }
    }
    else if(tar->auto_player && clients[tar->plyData[tar->nPlayer].i].fd == -1) 
        AutoPlay(tar, tar->nPlayer);
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

void bitw1(int* tar, int k) {
    *tar = ((*tar) | (1<<k));
    return;
}

bool bitis1(int tar, int k) {
    return (tar&(1<<k));
}
