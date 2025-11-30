#include "libserv.h"

#define DEBUG

struct pollfd clients[FOPEN_MAX];
int in_room[FOPEN_MAX]; //hash map
time_t lst_conn[FOPEN_MAX]; //last msg timestamp
Queue* q;

int main(int argc, char** argv) {
    int i, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    Rooms* room = malloc(3*sizeof(Rooms));
    
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
            if(difftime(curr_time, lst_conn[frontNode->i]) >= 60) {
                //timeout
                int sockfd = frontNode->sockfd;
                Close(sockfd);
                clients[frontNode->i].fd = -1;
                in_room[frontNode->i] = -1;
                pop(q);
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
                    Write(connfd, buf, strlen(buf));
                    if(!has_vacant_room) Close(connfd);
                    else {
                        clients[i].fd = connfd;
                        lst_conn[i] = time(NULL);
                        push(q, connfd, -1, i);
                    }
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
            if(!(clients[i].revents & (POLLRDNORM | POLLERR))) continue;
            if((n = read(sockfd, buf, MAXLINE)) <= 0) {
                if(n == 0 || errno == ECONNRESET) {
                    Delete(q, i);
                    clients[i].fd = -1;
                    if(in_room[i] != -1) {
                        ExitCli(i, &room[in_room[i]], in_room[i]);
                        in_room[i] = -1;
                    }
                    Close(sockfd);
                }
                else err_sys("read error");
                if(--nready <= 0) break;
                continue;
            }
            if(buf[0] == ' ') {
                push(q, sockfd, in_room[i], i);
                if(--nready <= 0) break;
                continue;
            }
            buf[n] = 0;
#ifdef DEBUG
            printf("Recv: %s\n", buf);
#endif
            lst_conn[i] = time(NULL);
            push(q, sockfd, in_room[i], i);
            //TODO
            if(!isValidStr(buf, n)) {
#ifdef DEBUG
                printf("Invalid string received\n");
#endif
                Delete(q, i);
                clients[i].fd = -1;
                if(in_room[i] != -1) {
                    ExitCli(i, &room[in_room[i]], in_room[i]);
                    in_room[i] = -1;
                }
                Close(sockfd);
                if(--nready <= 0) break;
                continue;
            }
            //not in room->choose room
            if(in_room[i] == -1) {
                char usrn[15];
                int dm, rID, pKey;
                sscanf(buf, "%d %d %s %d", &dm, &rID, usrn, &pKey);
                int errcd = -1;
                if(rID > 2 || rID < 0) {
                    Delete(q, i);
                    clients[i].fd = -1;
                    Close(sockfd);
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
                    JoinRoom(&room[rID], usrn, i);
                    char tmp[MAXLINE];
                    GetOneRoomInfo(&room[rID], rID, tmp);
                    SendAll(&room[rID], tmp, 1);
                    in_room[i] = rID;
                    //TODO
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
                else if(buf[0] == '2') 
                    Unlock(&room[rID], i);
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
                //play_card
                //TODO
                break;
            case 2:
                //bid
                //TODO
                break;
            case 3:
                //in room, status 3
                //continue game
                //TODO
                break;
            case 4:
                //in room, status 4
                //TODO
                break;
            default:
                err_quit("Unknown room status %d\n", room[rID].stat);
            }
            if(--nready <= 0) break;
        }
    }
}
