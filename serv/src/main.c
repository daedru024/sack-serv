#include "libserv.h"

int main(int argc, char** argv) {
    int i, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE];
    socklen_t clilen;
    struct pollfd clients[FOPEN_MAX];
    int in_room[FOPEN_MAX]; //hash map
    time_t lst_conn[FOPEN_MAX]; //last msg timestamp
    struct sockaddr_in cliaddr, servaddr;
    Rooms room[3] = malloc(3*sizeof(Rooms));
    
    for(int k=0; k<3; k++) init_RoomInfo(&room[k]);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if(bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) 
        err_sys("bind error");
    
    Listen(listenfd, LISTENQ);

    clients[0].fd = listenfd;
    clients[0].events = POLLRDNORM;
    for(i=1; i<FOPEN_MAX; i++) {
        clients[i].fd = -1;
        in_room[i] = -1;
    }
    maxi = 0;

    for( ; ; ) {
        nready = Poll(clients, maxi+1);
        if(clients[0].revents & POLLRDNORM) {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);

            for(i=1; i<FOPEN_MAX; i++) {
                if(clients[i].fd < 0) {
                    bool has_vacant_room = 0;
                    GetRoomInfo(&room[0], 0, buf);
                    char tmp[MAXLINE];
                    for(int j=1; j<3; j++) {
                        GetRoomInfo(&room[j], j, tmp);
                        strcat(buf, tmp);
                        if(room[j].stat == 0) has_vacant_room = 1;
                    }
                    Write(connfd, buf, strlen(buf));
                    if(!has_vacant_room) Close(connfd);
                    else clients[i].fd = connfd;
                    break;
                }
            }
            if(i == FOPEN_MAX) {
                Close(connfd);
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
                        // client reset conn
                        Close(sockfd);
                        clients[i].fd = -1;
                        if(in_room[i] != -1) {
                            ExitCli(sockfd, &room[in_room[i]], in_room[i]);
                            in_room[i] = -1;
                        }
                    }
                    else err_sys("read error");
                }
                else if(n == 0) {
                    Close(sockfd);
                    clients[i].fd = -1;
                    if(in_room[i] != -1) {
                        ExitCli(sockfd, &room[in_room[i]], in_room[i]);
                        in_room[i] = -1;
                    }
                }
                else {
                    //TODO
                    if(!isValidStr(buf, n)) {
                        Close(sockfd);
                        clients[i].fd = -1;
                        if(in_room[i] != -1) {
                            ExitCli(sockfd, &room[in_room[i]], in_room[i]);
                            in_room[i] = -1;
                        }
                    }
                    //not in room->choose room
                    if(in_room[i] == -1) {
                        char usrn[15];
                        int dm, rID, pKey;
                        sscanf(buf, "%d %d %s %d", &dm, &rID, usrn, &pKey);
                        int errcd = -1;
                        if(rID > 2 || rID < 0) {
                            Close(sockfd);
                            clients[i].fd = -1;
                        }
                        else if(room[rID].stat != 0) {
                            if(room[rID].stat == 4) {
                                if(room[rID].num_players<5) errcd = 1; // 1 Locked
                                else errcd = 0; // 0 Full
                            }
                            else errcd = 4; // 4 Playing
                            sprintf(usrn, "re %d", errcd);
                            Write(sockfd, usrn, strlen(usrn));
                        }
                        else if(pKey != room[rID].passkey) {
                            if(pKey == 10000) 
                                errcd = 2; // 2 Private
                            else {
                                errcd = 3; // 3 WrongPIN
                                //TODO: close conn if >=3 times
                            }
                            sprintf(usrn, "re %d", errcd);
                            Write(sockfd, usrn, strlen(usrn));
                        }
                        else {
                            //client joins room
                            JoinRoom(&room[rID], usrn, sockfd);
                            //TODO
                            //SendAll(&room[rID],)
                        }
                    }
                    //TODO
                    //in room, status 0 or 4
                    //in room, status 1
                    //in room, status 2
                    Write(sockfd, buf, n);
                }

                if(--nready <= 0) break;
            }
        }
    }
}
