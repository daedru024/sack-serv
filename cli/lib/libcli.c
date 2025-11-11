#include "libcli.h"

int Connect(const char *host, const char *serv) {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) err_quit("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    
    if(inet_pton(AF_INET, serv, &servaddr.sin_addr) <= 0)
        err_quit("inet_pton error");
}