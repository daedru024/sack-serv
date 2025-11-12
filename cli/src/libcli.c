#include "libcli.h"

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

int Close(int sockfd) {
    if(close(sockfd) == -1) {
        err_sys("close error");
        return -1;
    }
    return 0;
}

int PlayCard(int sockfd, int PlayerID, int Card, int MaskUc) {
    char buf[MAXLINE];
    sprintf(buf, "13 %d %d %d", PlayerID, Card, MaskUc);
    if(write(sockfd, buf, strlen(buf)) != strlen(buf)) {
        err_sys("write error");
        return -1;
    }
    return 0;
}

int Bid(int sockfd, int PlayerID, int amount, int rem_money) {
    char buf[MAXLINE];
    sprintf(buf, "17 %d %d %d", PlayerID, amount, rem_money);
    if(write(sockfd, buf, strlen(buf)) != strlen(buf)) {
        err_sys("write error");
        return -1;
    }
    return 0;
}

int Join(int sockfd, int RoomID, const char* username, int PIN) {
    char buf[MAXLINE];
    sprintf(buf, "11 %d %s %d", RoomID, username, PIN);
    if(write(sockfd, buf, strlen(buf)) != strlen(buf)) {
        err_sys("write error");
        return -1;
    }
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

int Lock(int sockfd) {
    if(write(sockfd, "3", 1) != 1) {
        err_sys("write error");
        return -1;
    }
    return 0;
}

int Privt(int sockfd, int PIN) {
    char buf[MAXLINE];
    sprintf(buf, "5 %d", PIN);
    if(write(sockfd, buf, strlen(buf)) != strlen(buf)) {
        err_sys("write error");
        return -1;
    }
    return 0;
}
