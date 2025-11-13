#include "libcli.h"

int Bid(int sockfd, int PlayerID, int amount, int rem_money) {
    char buf[MAXLINE];
    sprintf(buf, "17 %d %d %d", PlayerID, amount, rem_money);
    Write(sockfd, buf, strlen(buf));
    return 0;
}

int Close(int sockfd) {
    if(close(sockfd) == -1) {
        err_sys("close error");
        return -1;
    }
    return 0;
}

int Connect(const char *servip) {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        err_sys("socket error");
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    
    if(inet_pton(AF_INET, servip, &servaddr.sin_addr) <= 0) {
        err_sys("inet_pton error");
        return -1;
    }
    
    if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        err_sys("connect error");
        return -1;
    }
    
    return sockfd;
}

int Join(int sockfd, int RoomID, const char* username, int PIN) {
    char buf[MAXLINE];
    sprintf(buf, "11 %d %s %d", RoomID, username, PIN);
    Write(sockfd, buf, strlen(buf));
    return 0;
}

int Lock(int sockfd) {
    Write(sockfd, "3", 1);
    return 0;
}

int PlayCard(int sockfd, int PlayerID, int Card, int MaskUc) {
    char buf[MAXLINE];
    sprintf(buf, "13 %d %d %d", PlayerID, Card, MaskUc);
    Write(sockfd, buf, strlen(buf));
    return 0;
}

int Privt(int sockfd, int PIN) {
    char buf[MAXLINE];
    sprintf(buf, "5 %d", PIN);
    Write(sockfd, buf, strlen(buf));
    return 0;
}

int Recv(int sockfd, char* recvline) {
    int n;
    n = read(sockfd, recvline, MAXLINE);
    if(n == -1) {
        err_sys("read error");
        return -1;
    }
    recvline[n] = 0;
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
