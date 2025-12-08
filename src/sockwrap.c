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

int Close(int idx) {
    int n;
    if(idx < 0 || clients[idx].fd < 0) return 0;
    Delete(q, idx);
    in_room[idx] = -1;
    if(close(clients[idx].fd) == -1) {
        if(errno == EBADF) {
            clients[idx].fd = -1;
            return -1;
        }
        err_sys("close error");
        return -1;
    }
    clients[idx].fd = -1;
    return 0;
}

int Closefd(int sockfd) {
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

void SendAll(Rooms* tar, char* msg, int c) {
    //c: 1 print id, 2 from exitcli, 3 from exitcli and print id, 10 if ignore player 0
    char tmp[MAXLINE];
    int k = (c == 10);
    for(int i=k; i<tar->num_players; i++) {
        if(tar->plyData[i].i == -1) continue;
        if(c == 1 || c == 3)
            sprintf(tmp, "%s %d \n", msg, i);
        else {
            strcpy(tmp, msg);
            if(tmp[0] != 'a') strcpy(tar->LastBroadcast, tmp);
        }
        if(Write(clients[tar->plyData[i].i].fd, tmp, strlen(tmp)) == -1) {
            if(c != 2) {
                ExitCli(tar->plyData[i].i, tar, in_room[tar->plyData[i].i], i);
                if(c == 3) return;
                continue;
            }
            Close(tar->plyData[i].i);
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