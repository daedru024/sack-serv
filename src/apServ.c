#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

#define DEBUG
#define	LISTENQ	1024
#define	MAXLINE	4096
#define	SERV_PORT 9877

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {}
    return;
}

void err_sys(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap); 
    fprintf(stderr, ": %s\n", strerror(errno)); 
    va_end(ap);
    exit(1);
}

void Listen(int sockfd, int backlog) {
    char *ptr;
    if ( (ptr = getenv("LISTENQ")) != NULL)
        backlog = atoi(ptr);

    if (listen(sockfd, backlog) < 0)
        err_sys("listen error");
}

int Recv(int sockfd, char *recvline) {
    fd_set rfds;
    struct timeval tv;
    int sel;
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    sel = select(sockfd + 1, &rfds, NULL, NULL, &tv);
    if (sel < 0) {
        if (errno == EINTR) return -1; 
        err_sys("Select");
        return -1;
    }
    else if (sel == 0) return -2;
    if (FD_ISSET(sockfd, &rfds)) {
        ssize_t n = recv(sockfd, recvline, MAXLINE - 1, 0);
        if (n < 0) err_sys("Recv");
        else if (n == 0) printf("Connection closed\n");
        recvline[n] = 0;
#ifdef DEBUG
        printf("Recv: %s\n", recvline);
#endif
        return (int)n;
    }
    return -1;
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
#ifdef DEBUG
    printf("Sent: %s\n", (char*)vptr);
#endif
    return;
}

void bitw1(int* tar, int k) {
    *tar = ((*tar) | (1<<k));
    return;
}

bool bitis1(int tar, int k) {
    return (tar&(1<<k));
}

void PlayCard(int sockfd, int* MASK_Uc, int playerID) {
    char buf[MAXLINE];
    int r = rand() % (10-(__builtin_popcount(*MASK_Uc)));
    for(int i=0; i<10; i++) {
        if(bitis1(*MASK_Uc, i)) continue;
        if(--r == 0) {
            sprintf(buf, "13 %d %d %d", playerID, i, *MASK_Uc);
            bitw1(MASK_Uc, i);
            Write(sockfd, buf, strlen(buf));
        }
    }
}

void AutoPlay(int sockfd) {
    //TODO
    //msg format: {playerID} {num_players} {MASK_Uc} {rem_money} {sPlayer} {aban} {LastBroadcast}
    char buf[1000];
    char tmp[MAXLINE];
    int n, playerID, num_players, MASK_Uc, rem_money, sPlayer, aban;
    n = Recv(sockfd, tmp);
    tmp[n] = 0;
    sscanf(tmp, "%d %d %d %d %d %d", &playerID, &num_players, &MASK_Uc, &rem_money, &sPlayer, &aban);
    int lst_player = (playerID-1+num_players) % num_players;
    int refund[5] = {2, 3, 4, 6, 0};
    if(num_players == 4) {
        refund[1] = 4;
        refund[2] = 6;
        refund[3] = 0;
    }
    char* pos = strstr(tmp, "\n");
    n = strlen(tmp)-(pos-tmp);
    strncpy(buf, pos, n);
    do {
        if(n == -2) continue;
        buf[n] = 0;
#ifdef DEBUG
        printf("msg: %s\n", buf);
#endif
        //c {PlayerID} {code}
        if(buf[0] == 'c') {
            int pID, cd;
            sscanf(buf, "c %d %d", &pID, &cd);
            if(cd == -1 || cd == 0) continue;
            else if(pID == lst_player) {
                if(playerID != sPlayer) PlayCard(sockfd, &MASK_Uc, playerID);
                else {
                    aban = 0;
                    //bid
                    sprintf(buf, "17 %d 0 %d", playerID, rem_money);
                    rem_money += refund[aban];
                    Write(sockfd, buf, strlen(buf));
                }
            }
            continue;
        } 
        if(buf[0] == 'a') continue;
        if(buf[0] == 'b') {
            int pID;
            if(buf[1] == 'e') {
                //be {PlayerID} {amount} {sPlayer} {last_card}
                sscanf(buf, "be %d %d %d %d", &pID, &pID, &sPlayer, &pID);
                if(MASK_Uc == 1023) {
                    close(sockfd);
                    return;
                }
                if(sPlayer == playerID) PlayCard(sockfd, &MASK_Uc, playerID);
                continue;
            }
            //b {PlayerID} {amount} {nPlayer} {card}
            int amount, nPlayer, cd;
            sscanf(buf, "b %d %d %d %d", &pID, &amount, &nPlayer, &cd);
            if(pID != playerID && cd != -1) aban++;
            if(nPlayer == playerID) {
                //bid
                //17 {PlayerID} 0 {rem_money} 
                sprintf(buf, "17 %d 0 %d", playerID, rem_money);
                rem_money += refund[aban];
                Write(sockfd, buf, strlen(buf));
            }
        }
    } while((n = Recv(sockfd, buf)) != 0);
    return;
}

int main(int argc, char **argv) {
    int listenfd, connfd, connfd2;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT+1);

    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(listenfd);
        exit(1);
    }

    if(bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) 
        err_sys("bind error");

    Listen(listenfd, LISTENQ);
    signal(SIGCHLD, sig_chld);
    for ( ; ; ) {
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR) continue;
            else err_sys("accept");
        }
        if ((childpid = fork()) == 0) {
            close(listenfd);
            AutoPlay(connfd);
            exit(0);
        }
        close(connfd);
    }
}
