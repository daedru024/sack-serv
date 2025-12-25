#include "libserv.h"

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map
extern const int Cards[10];

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
    bitw1(&tar->plyData[k].MASK_Uc, c);
    char tmp[10];
    if(tar->auto_player && tar->num_players == 4 && tar->plyData[k].i == -1) {
        //choose rabbit for host
        tar->rabbit[0] = rand()%10;
        bitw1(&tar->plyData[0].MASK_Uc, tar->rabbit[0]);
        sprintf(tmp, "ri %d\n", tar->rabbit[0]);
        if(Write(clients[tar->plyData[0].i].fd, tmp, strlen(tmp)) == -1) {
            ExitCli(tar->plyData[0].i, tar, in_room[tar->plyData[0].i], 0);
            return;
        }
        SendAll(tar, "START_ROUND\n", 10);
        if(tar->stat == 0) return;
        tar->nPlayer = 0;
        tar->sPlayer = 0;
        tar->rnd = 1;
        ApConnect(tar, 3, in_room[tar->plyData[0].i]);
        return;
    }
    sprintf(tmp, "ri %d\n", c);
    if(Write(clients[tar->plyData[k].i].fd, tmp, strlen(tmp)) == -1) {
        ExitCli(tar->plyData[k].i, tar, in_room[tar->plyData[k].i], k);
        return;
    }
    if(k == 0) {
        SendAll(tar, "START_ROUND\n", 10);
        if(tar->stat == 0) return;
        tar->nPlayer = 0;
        tar->sPlayer = 0;
        tar->rnd = 1;
    }
}

void RecvBid(Rooms* tar, char* msg) {
    //17 {PlayerID} {amount} {rem_money} 
    if(msg[0] != '1' || msg[1] != '7') return;
    int pID, pri, rem, cd = -1, nply = 0;
    char buf[MAXLINE];
    sscanf(msg, "17 %d %d %d", &pID, &pri, &rem);
    //b {PlayerID} {amount} {nPlayer} {card}
    if(pID != tar->nPlayer || (pri <= tar->lstbid && pri != 0)) nply = -1;
    else if(rem != tar->plyData[pID].rem_money) {
        nply = -1;
    }
    else if(pri == 0) {
        tar->plyData[pID].rem_money += tar->abdMoney[tar->aban];
        tar->plyData[pID].LastBid = -1;
        cd = tar->stks[tar->rnd-1][(tar->sPlayer+tar->aban)%tar->num_players];
        tar->aban++;
    }
    else {
        tar->lstbid = pri;
        tar->plyData[pID].LastBid = pri;
        tar->wonstk[tar->rnd-1] = pID;
    }
    if(nply == 0) {
        tar->nPlayer = (tar->nPlayer+1) % tar->num_players;
        while(tar->nPlayer != pID) {
            if(tar->plyData[tar->nPlayer].LastBid != -1) break;
            tar->nPlayer = (tar->nPlayer+1) % tar->num_players;
        }
        nply = tar->nPlayer;
    }
    sprintf(buf, "b %d %d %d %d\n", pID, pri, nply, cd);
    int k = (nply == -1) ? 20 : 0;
    SendAll(tar, buf, k);
    if(tar->stat == 0) return;
    if(nply == -1 && rem != tar->plyData[pID].rem_money) 
        ExitCli(tar->plyData[pID].i, tar, in_room[tar->plyData[pID].i], pID);
    if(tar->aban >= tar->num_players-1 && tar->plyData[tar->nPlayer].LastBid != 0) {
        //end bid
        pri = -1;
        for(int i=0; i<tar->num_players; i++) {
            if(tar->plyData[i].LastBid > 0) {
                tar->plyData[i].rem_money -= tar->plyData[i].LastBid;
                pri = i;
            }
            tar->plyData[i].LastBid = 0;
        }
        //be {PlayerID} {amount} {sPlayer} {last_card}
        cd = tar->stks[tar->rnd-1][(tar->sPlayer+tar->num_players-1)%tar->num_players];
        tar->sPlayer = (tar->sPlayer+1) % tar->num_players;
        sprintf(buf, "be %d %d %d %d\n", pri, tar->lstbid, tar->sPlayer, cd);
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
}

void RecvPlay(Rooms* tar, char* msg) {
    //13 {PlayerID} {cardID} {MaskUc}
    int pID, cID, mskUc;
    if(msg[0] != '1' || msg[1] != '3') return;
    sscanf(msg, "13 %d %d %d", &pID, &cID, &mskUc);
    int cd;
    char buf[MAXLINE];
    if(pID != tar->nPlayer) cd = 0;
    else if(mskUc != tar->plyData[pID].MASK_Uc || bitis1(mskUc, cID)) cd = -1;
    else {
        bitw1(&(tar->plyData[pID].MASK_Uc), cID);
        cd = 1;
        tar->stks[tar->rnd-1][pID] = cID;
        if(cID >= 8) {
            if(tar->values[tar->rnd-1] >= 8 || tar->values[tar->rnd-1] == -1) tar->values[tar->rnd-1] = -1;
            else tar->values[tar->rnd-1] = cID;
        }
    }
    //c {PlayerID} {code}
    sprintf(buf, "c %d %d\n", pID, cd);
    int k = (cd == 0) ? 0 : 20;
    SendAll(tar, buf, k);
    if(tar->stat == 0) return;
    if(cd == 1) tar->nPlayer = (tar->nPlayer+1) % tar->num_players;
    else if(cd == -1) ExitCli(tar->plyData[pID].i, tar, in_room[tar->plyData[pID].i], pID);

    if(tar->nPlayer == tar->sPlayer) {
        tar->stat = 2;
        switch(tar->values[tar->rnd-1]) {
        case -1:
            //ignore all dogs
            tar->values[tar->rnd-1] = 0;
            for(int i=0; i<tar->num_players; i++) {
                int j = tar->stks[tar->rnd-1][i];
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
                if(j >= 8) continue;
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
                if(j >= 8) continue;
                tar->values[tar->rnd-1] += Cards[j];
                if(Cards[j] < mn) mn = Cards[j];
            }
            tar->values[tar->rnd-1] -= mn;
            break;
        default:
            tar->values[tar->rnd-1] = 0;
            for(int i=0; i<tar->num_players; i++) {
                int j = tar->stks[tar->rnd-1][i];
                tar->values[tar->rnd-1] += Cards[j];
            }
        }
    }
}