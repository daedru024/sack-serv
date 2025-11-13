#include "libserv.h"

int main(int argc, char** argv) {
    int i, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE];
    socklen_t clilen;
    struct pollfd clients[FOPEN_MAX];
    struct sockaddr_in cliaddr, servaddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_quit("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if(bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) 
        err_quit("bind error");
    
    Listen(listenfd, LISTENQ);

    clients[0].fd = listenfd;
    clients[0].events = POLLRDNORM;
    for(i=1; i<FOPEN_MAX; i++) 
        clients[i].fd = -1;
    maxi = 0;

    for( ; ; ) {
        nready = Poll(clients, maxi+1);
        if(clients[0].revents & POLLRDNORM) {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);

            for(i=1; i<FOPEN_MAX; i++) {
                if(clients[i].fd < 0) {
                    clients[i].fd = connfd;
                    break;
                }
            }
            if(i == FOPEN_MAX) {
                close(connfd);
                printf("Too many clients\n");
                continue;
            }

            clients[i].events = POLLRDNORM;
            if(i > maxi) maxi = i;
            if(--nready <= 0) continue;
        }

        for(i=1; i<=maxi; i++) {
            if((sockfd = clients[i].fd) < 0) continue;
            if(clients[i].revents & (POLLRDNORM | POLLERR)) {
                if((n = read(sockfd, buf, MAXLINE)) < 0) {
                    if(errno == ECONNRESET) {
                        Close(sockfd);
                        clients[i].fd = -1;
                    }
                    else err_sys("read error");
                }
                else if(n == 0) {
                    Close(sockfd);
                    clients[i].fd = -1;
                }
                else Write(sockfd, buf, n);

                if(--nready <= 0) break;
            }
        }
    }
}