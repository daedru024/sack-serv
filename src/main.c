#include "libserv.h"

struct pollfd clients[FOPEN_MAX];
int in_room[FOPEN_MAX]; //hash map
time_t lst_conn[FOPEN_MAX]; //last msg timestamp
Queue* q;
int maxi;

const int AbdMoney[3][5] = {{3,6,0,0,0},
                            {2,4,6,0,0},
                            {2,3,4,6,0}};

const int Cards[10] = {-8,-5,0,3,5,8,11,15,-9,9};

int main(int argc, char** argv) {
    int i, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    Rooms* room = malloc(3*sizeof(Rooms));
    srand(time(NULL));
    
    for(int k=0; k<3; k++) init_RoomInfo(&room[k]);

    q = malloc(sizeof(Queue));
    init_Queue(q);

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
        while(!isEmpty(q)) {
            Node* frontNode = front(q);
            time_t curr_time = time(NULL);
            if(clients[frontNode->i].fd != frontNode->sockfd) {
                //client disconnected
                pop(q);
                continue;
            }
            if(difftime(curr_time, lst_conn[frontNode->i]) > 90) {
                //timeout
                int sockfd = frontNode->sockfd;
#ifdef DEBUG
                printf("Timeout %d\n", i);
#endif
                if(in_room[frontNode->i] == -1) ExitCli(frontNode->i, NULL, -1, -1);
                else ExitCli(frontNode->i, &room[in_room[frontNode->i]], in_room[frontNode->i], -1);
                continue;
            }
            break;
        }
        nready = Poll(clients, maxi+1);
        if(clients[0].revents & POLLRDNORM) {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);

            for(i=1; i<FOPEN_MAX; i++) {
                if(clients[i].fd < 0) {
                    bool has_vacant_room = (room[0].stat == 0);
                    GetRoomInfo(&room[0], 0, buf);
                    char tmp[MAXLINE];
                    for(int j=1; j<3; j++) {
                        GetRoomInfo(&room[j], j, tmp);
                        strcat(buf, tmp);
                        if(room[j].stat == 0) has_vacant_room = 1;
                    }
                    strcat(buf, "\n");
                    Write(connfd, buf, strlen(buf));
                    if(!has_vacant_room) Closefd(connfd);
                    else {
                        clients[i].fd = connfd;
                        lst_conn[i] = time(NULL);
                        push(q, connfd, -1, i);
                    }
                    break;
                }
            }
            if(i == FOPEN_MAX) {
                Closefd(connfd);
                printf("Too many clients\n");
                continue;
            }

            clients[i].events = POLLRDNORM;
            if(i > maxi) maxi = i;
            if(--nready <= 0) continue;
        }

        for(i=1; i<=maxi; i++) {
            if((sockfd = clients[i].fd) < 0) continue;
            if(!(clients[i].revents & (POLLRDNORM | POLLERR))) continue;
            if((n = read(sockfd, buf, MAXLINE)) <= 0) {
                if(n == 0 || errno == ECONNRESET) {
#ifdef DEBUG
                    if(n == 0) printf("Connection closed\n");
                    else printf("RST\n");
#endif
                    if(in_room[i] == -1) ExitCli(i, NULL, -1, -1);
                    else ExitCli(i, &room[in_room[i]], in_room[i], -1);
                }
                else err_msg("read error");
                if(--nready <= 0) break;
                continue;
            }
            if(buf[0] == ' ') { // Heartbeat should only work when waiting in public room
#ifdef DEBUG
                printf("Heartbeat from %d\n", i);
#endif
                if(in_room[i] == -1) ;
                else if(room[in_room[i]].stat<4 && room[in_room[i]].stat>0) ;
                else if(room[in_room[i]].passkey != 10000 || room[in_room[i]].stat == 4) {
                    time_t currtime = time(NULL);
                    if(difftime(currtime, room[in_room[i]].madePriv) >= 600) {
                        if(room[in_room[i]].passkey != 10000) CloseRoom(&room[in_room[i]]);
                        else ExitCli(room[in_room[i]].plyData[0].i, NULL, -1, -1);
                    }
                    else push(q, sockfd, in_room[i], i);
                }
                else push(q, sockfd, in_room[i], i);
                if(--nready <= 0) break;
                continue;
            }
            buf[n] = 0;
#ifdef DEBUG
            printf("Recv: %s\n", buf);
#endif
            lst_conn[i] = time(NULL);
            push(q, sockfd, in_room[i], i);
            if(!isValidStr(buf, n)) {
#ifdef DEBUG
                printf("Invalid string received\n");
#endif
                if(in_room[i] == -1) ExitCli(i, NULL, -1, -1);
                else ExitCli(i, &room[in_room[i]], in_room[i], -1);
                if(--nready <= 0) break;
                continue;
            }
            //not in room->choose room
            if(in_room[i] == -1) {
                char usrn[15];
                int dm, rID, pKey;
                sscanf(buf, "%d %d %s %d", &dm, &rID, usrn, &pKey);
                if(dm != 11) continue;
                int errcd = -1;
                if(rID > 2 || rID < 0) ExitCli(i, NULL, -1, -1);
                else if(room[rID].stat != 0) {
                    if(room[rID].stat == 4) {
                        if(room[rID].num_players<5) errcd = 1; // 1 Locked
                        else errcd = 0; // 0 Full
                    }
                    else errcd = 4; // 4 Playing
                    sprintf(usrn, "re %d\n", errcd);
                    Write(sockfd, usrn, strlen(usrn));
                }
                else if(pKey != room[rID].passkey) {
                    if(pKey == 10000) 
                        errcd = 2; // 2 Private
                    else 
                        errcd = 3; // 3 WrongPIN
                    sprintf(usrn, "re %d\n", errcd);
                    Write(sockfd, usrn, strlen(usrn));
                }
                else {
                    //client joins room
                    JoinRoom(&room[rID], usrn, i);
                    char tmp[MAXLINE];
                    GetOneRoomInfo(&room[rID], rID, tmp);
                    SendAll(&room[rID], tmp, 1);
                    in_room[i] = rID;
                }
                if(--nready <= 0) break;
                continue;
            }
            int rID = in_room[i];
            switch (room[rID].stat) {
            case 0:
                //possibilities: 
                //lock/unlock
                if(buf[0] == '3') 
                    Lock(&room[rID], i);
                //choose color
                else if(buf[0] == '7') 
                    ChooseColor(&room[rID], i, buf);
                //make private/public
                else if(buf[0] == '5') {
                    int k = 0;
                    for(int j=0; j<3; j++) 
                        if(room[j].passkey != 10000) k++;
                    MakePrivate(&room[rID], i, buf, k);
                }
                break;
            case 1:
                if(room[in_room[i]].rnd == 0) Rabbit(&room[in_room[i]], i, buf);
                else RecvPlay(&room[in_room[i]], buf);
                break;
            case 2:
                RecvBid(&room[in_room[i]], buf);
                if(room[in_room[i]].stat == 3) GetScore(&room[in_room[i]]);
                break;
            case 3:
                //in room, status 3
                //continue game
                break;
            case 4:
                if(buf[0] == '7') 
                    ChooseColor(&room[rID], i, buf);
                else if(buf[0] == '3') 
                    Lock(&room[rID], i);
                else if(buf[0] == '2') 
                    Unlock(&room[rID], i);
                break;
            default:
                err_msg("Unknown room status %d\n", room[rID].stat);
            }
            if(--nready <= 0) break;
        }
    }
}
