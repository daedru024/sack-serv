#include "libserv.h"

#define DEBUG

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map

int Accept(int sockfd, struct sockaddr* sa, socklen_t *ptr) {
    int n;
again:
    if((n = accept(sockfd, sa, ptr)) < 0) {
        if(errno == ECONNABORTED)
            goto again;
        else 
            err_sys("accept error");
    }
    return n;
}

int Close(int sockfd) {
    for(int i=1; i<FOPEN_MAX; i++) {
        if(clients[i].fd == sockfd) {
            clients[i].fd = -1;
            in_room[i] = -1;
            break;
        }
    }
    if(close(sockfd) == -1) {
        if(errno == EBADF) return -1;
        err_sys("close error");
        return -1;
    }
    return 0;
}

void Listen(int sockfd, int backlog) {
    char *ptr;

    if ( (ptr = getenv("LISTENQ")) != NULL)
        backlog = atoi(ptr);

    if (listen(sockfd, backlog) < 0)
        err_sys("listen error");
}

int Poll(struct pollfd *fdarr, unsigned long nfds) {
    int n;
    if((n = poll(fdarr, nfds, -1)) < 0) 
        err_sys("poll error");
    return n;
}

void SendAll(Rooms* tar, char* msg) {
    for(int i=0; i<tar->num_players; i++) {
        if(tar->plyData[i].sockfd == -1) continue;
        //sprintf(msg, "%s%d ", msg, i);
        if(Write(tar->plyData[i].sockfd, msg, strlen(msg)) == -1) {
            Close(tar->plyData[i].sockfd);
            tar->plyData[i].sockfd = -1;
            continue;
        }
        //Write(tar->plyData[i].sockfd, "%d ", i);
    }
    return;
}

int Write(int sockfd, const void *vptr, size_t n) {
    size_t rem;
    ssize_t nw;
    const char *ptr = vptr;
    rem = n;
    while(rem>0) {
        if((nw = send(sockfd, ptr, rem, MSG_NOSIGNAL)) <= 0) {
            if(nw<0 && errno == EINTR) continue;
            if(nw == 0) return -1;
            else {
                err_sys("Write error");
                return -1;
            }
        }
        rem -= nw;
        ptr += nw;
    }
#ifdef DEBUG
    printf("Sent: %s\n", (char*)vptr);
#endif
    return 0;
}



void err_msg(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    return;
}

void err_quit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void err_sys(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap); 
    fprintf(stderr, ": %s\n", strerror(errno)); 
    va_end(ap);
    exit(1);
}



void ExitCli(int sockfd, Rooms* Rm, int rID) {
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
            if(Rm->plyData[i].sockfd == sockfd) {
                Rm->plyData[i] = Rm->plyData[i+1];
                Rm->plyData[i+1].sockfd = sockfd;
            }
        }
        GetOneRoomInfo(Rm, rID, tmp);
        SendAll(Rm, tmp);
        return;
    }
    char tmp[MAXLINE];
    for(int i=0; i<Rm->num_players; i++) {
        if(Rm->plyData[i].sockfd == sockfd) {
            pID = i;
            break;
        }
    }
    if(Rm->stat == 1 || Rm->stat == 2) {
        if(Rm->auto_player) {
            char tmp[MAXLINE];
            sprintf(tmp, "ap %d\n", pID);
            SendAll(Rm, tmp);
            for(int i=0; i<Rm->num_players; i++) 
                Close(Rm->plyData[i].sockfd);
            init_RoomInfo(Rm);
            return;
        }
        else {
            sprintf(tmp, "ap %d\n", pID);
            SendAll(Rm, tmp);

            //TODO auto play mech
            return;
        }
    }
    if(Rm->stat == 3) return;
}

void GetOneRoomInfo(Rooms* tar, int rID, char* ret) {
    //in {RoomID} {n_Players} {username[:] color[:]} {locked} {PIN} {playerID}
    char tmp[20];
    sprintf(ret, "in %d %d ", rID, tar->num_players);
    for(int i=0; i<tar->num_players; i++) {
        sprintf(tmp, "%s %d ", tar->plyData[i].username, tar->plyData[i].col);
        strcat(ret, tmp);
    }
    sprintf(ret, "%s%d %d ", ret, (tar->stat==4 && tar->rnd==0), tar->passkey);
    return;
}

void GetRoomInfo(Rooms* tar, int rID, char* ret) {
    if(tar->stat == 0) {
        // available
        sprintf(ret, "ra %d %d ", rID, tar->num_players);
        for(int i=0; i<tar->num_players; i++) {
            sprintf(ret, "%s %s %d ", ret, tar->plyData[i].username, tar->plyData[i].col);
        }
        if(tar->passkey == 10000) sprintf(ret, "%s0 ", ret);
        else sprintf(ret, "%s1 ", ret);
    }
    else // unavailable
        sprintf(ret, "ru %d %d %d ", rID, tar->num_players, tar->rnd);
    return;
}

void init_RoomInfo(Rooms* tar) {
    tar->stat = 0;
    tar->passkey = 10000;
    tar->num_players = 0;
    tar->rnd = 0;
    memset(tar->stks, 0, 45*sizeof(int));
    memset(tar->rabbit, 2, 5*sizeof(int));
    memset(tar->wonstk, -1, 9*sizeof(int));
    tar->sPlayer = 0;
    tar->auto_player = 0;
    for(int i=0; i<5; i++) {
        bzero(&(tar->plyData[i]), sizeof(Player));
        tar->plyData[i].col = -1;
        tar->plyData[i].rem_money = 15;
    }
    return;
}

bool isValidStr(char* tar, int n) {
    tar[n] = 0;
    if(n > 1 && tar[0] == '1' && tar[1] == '1') {
        //11 {RoomID} {username} {PIN}
        if(!isdigit(tar[3])) return 0;
        for(int i=n-5; i<n; i++) 
            if(!isdigit(tar[i])) return 0;
        return 1;
    }
    for(int i=0; i<n; i++) 
        if(!isdigit(tar[i])) return 0;
    return 1;
}

void JoinRoom(Rooms* tar, char* usrn, int fd) {
    int playerID = tar->num_players;
    strcpy(tar->plyData[playerID].username, usrn);
    tar->plyData[playerID].sockfd = fd;
    if(++tar->num_players == 5) tar->stat = 4;
    return;
}


void bitw1(int* tar, int k) {
    *tar = ((*tar) | (1<<k));
    return;
}

bool bitis1(int tar, int k) {
    return (tar&(1<<k));
}
