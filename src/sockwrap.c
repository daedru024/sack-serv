#include "libserv.h"

#define DEBUG

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map
extern time_t lst_conn[FOPEN_MAX]; //last msg timestamp
extern Queue* q;

/**** Elem functions ****/

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
    int n;
    if(sockfd < 0) return 0;
    if(close(sockfd) == -1) {
        if(errno == EBADF) return -1;
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
    if((n = poll(fdarr, nfds, 5000)) < 0) 
        err_sys("poll error");
    return n;
}

void SendAll(Rooms* tar, char* msg, bool pID) {
    char tmp[MAXLINE];
    for(int i=0; i<tar->num_players; i++) {
        if(tar->plyData[i].i == -1) continue;
        if(pID)
            sprintf(tmp, "%s %d ", msg, i);
        else
            sprintf(tmp, "%s", msg);
        if(Write(clients[tar->plyData[i].i].fd, tmp, strlen(tmp)) == -1) {
            ExitCli(tar->plyData[i].i, tar, in_room[tar->plyData[i].i]);
            Close(clients[tar->plyData[i].i].fd);
            tar->plyData[i].i = -1;
            continue;
        }
        char tmp[3];
        lst_conn[tar->plyData[i].i] = time(NULL);
        push(q, clients[tar->plyData[i].i].fd, in_room[tar->plyData[i].i], tar->plyData[i].i);
    }
    return;
}

int Write(int sockfd, const void *vptr, size_t n) {
    size_t rem;
    ssize_t nw;
    const char *ptr = vptr;
    rem = n;
    while(rem>0) {
        if((nw = send(sockfd, ptr, rem, MSG_NOSIGNAL)) <= 0) {
            if(nw<0 && errno == EINTR) continue;
            if(nw == 0) return -1;
            else {
                err_sys("Write error");
                return -1;
            }
        }
        rem -= nw;
        ptr += nw;
    }
#ifdef DEBUG
    printf("Sent: %s\n", (char*)vptr);
#endif
    return 0;
}