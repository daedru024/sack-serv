#include "libserv.h"

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
    if(close(sockfd) == -1) {
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

void Write(int sockfd, const void *vptr, size_t n) {
    size_t rem;
    ssize_t nw;
    const char *ptr = vptr;
    rem = n;
    while(rem>0) {
        if((nw = write(sockfd, ptr, rem)) <= 0) {
            if(nw<0 && errno == EINTR) nw = 0;
            else {
                err_sys("Write error");
                return;
            }
        }
        rem -= nw;
        ptr += nw;
    }
    return;
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
        GetRoomInfo(Rm, rID, tmp);
        for(int i=0; i<Rm->num_players; i++) 
            Write(Rm->plyData[i].sockfd, tmp, strlen(tmp));
        return;
    }
    if(Rm->stat == 1 || Rm->stat == 2) {
        //
    }
    //  2. stat==1 || 2
    //      auto_play
    //  3. stat==3
    //      do_nothing
}

void GetRoomInfo(Rooms* tar, int rID, char* ret) {
    char tmp[20];
    if(tar->stat == 0) {
        // available
        sprintf(ret, "ra %d %d ", rID, tar->num_players);
        for(int i=0; i<tar->num_players; i++) {
            sprintf(tmp, "%s %d ", tar->plyData[i].username, tar->plyData[i].col);
            strcat(ret, tmp);
        }
        if(tar->passkey == 10000) strcat(ret,"0 ");
        else strcat(ret, "1 ");
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

void bitw1(int* tar, int k) {
    *tar = ((*tar) | (1<<k));
    return;
}

bool bitis1(int tar, int k) {
    return (tar&(1<<k));
}