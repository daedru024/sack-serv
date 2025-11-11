#include "libcli.h"

int Connect(const char *servip) {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) 
        err_quit("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    
    if(inet_pton(AF_INET, servip, &servaddr.sin_addr) <= 0)
        err_quit("inet_pton error");
    
    if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
        err_quit("connect error");
    
    return sockfd;
}

int Close(int sockfd) {
    if(close(sockfd) == -1)
        err_sys("close error");
    return 0;
}

int PlayCard(int sockfd, int PlayerID, int Card, int MaskUc) {
    char buf[MAXLINE];
    sprintf(buf, "pc %d %d %d", PlayerID, Card, MaskUc);
    if(write(sockfd, buf, strlen(buf)) != strlen(buf)) {
        err_sys("write error");
        return -1;
    }
    return 0;
}

int Bid(int sockfd, int PlayerID, int amount, int rem_money) {
    char buf[MAXLINE];
    sprintf(buf, "b %d %d %d", PlayerID, amount, rem_money);
    if(write(sockfd, buf, strlen(buf)) != strlen(buf)) {
        err_sys("write error");
        return -1;
    }
    return 0;
}

int Join(int sockfd, int RoomID, const char* username, int PIN) {
    char buf[MAXLINE];
    sprintf(buf, "j %d %s %d", RoomID, username, PIN);
    if(write(sockfd, buf, strlen(buf)) != strlen(buf)) {
        err_sys("write error");
        return -1;
    }
    return 0;
}

int Recv(int sockfd, char* recvline) {
    int n;
    n = read(sockfd, recvline, MAXLINE);
    if(n == -1)
        err_sys("read error");
    recvline[n] = 0;
    // FORMAT: (reading with sstream might be better)

    // Start   game: "GAME_START"
    // Played  card: "c" PlayerID 1(played)/-1(error)
    // Before   bid: "BID" NextPlayerID
    // After    bid: "b" PlayerID amount "BID" NextPlayerID
    // End      bid: "be" PlayerID amount
    // End     game: "WON_STACK" stk[i][j] won[i] score[i] WinnerID
    //               if stk[i][j] was rabbit then stk[i][j] = -rabbit[k]

    // Room    info: "r"  Room[i] n_Players username[i] 1(need PIN)/0(public)
    // Rooms   full: "rf" Room[i] n_Players username[i] 1(need PIN)/0(public)
    // Room details: "rd" Room[i] n_Players username[i] color[i]
    // Room unavail: "re" 0(Full)/1(Locked)/2(Private)/3(WrongPIN)
    // Color chosen: "ce" n_Players username[i] color[i]
    return n;
}