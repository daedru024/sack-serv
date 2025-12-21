#include "libserv.h"

extern struct pollfd clients[FOPEN_MAX];
extern int in_room[FOPEN_MAX]; //hash map
extern Queue* q;

/**** Elem functions ****/

int Accept(int sockfd, struct sockaddr* sa, socklen_t *ptr) {
    int n;
#ifndef _WIN32
again:
#endif
    if((n = accept(sockfd, sa, ptr)) < 0) {
#ifndef _WIN32
        if(errno == ECONNABORTED)
           goto again;
        else 
#endif
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
#ifndef _WIN32
        if(errno == EBADF) {
           clients[idx].fd = -1;
           return -1;
        }
        err_sys("close error");
        return -1;
#endif
    }
    clients[idx].fd = -1;
    return 0;
}

int Closefd(int sockfd) {
    int n;
    if(sockfd < 0) return 0;
    if(close(sockfd) == -1) {
#ifndef _WIN32
        if(errno == EBADF) return -1;
        err_sys("close error");
        return -1;
#endif
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

int Write(int sockfd, const void *vptr, size_t n) {
    size_t rem;
    ssize_t nw;
    const char *ptr = vptr;
    // char debug_buf[4096];
    // if (n < 4096) {
    //     memcpy(debug_buf, vptr, n);
    //     debug_buf[n] = '\0';
    //     // 使用 [] 包起來以便觀察有沒有多餘的空白或換行
    //     printf("[DEBUG SEND]: [%s]\n", debug_buf); 
    //     fflush(stdout); // 強制立刻印出，不要緩衝
    // }
    rem = n;
    while(rem>0) {
        if((nw = send(sockfd, ptr, rem, MSG_NOSIGNAL)) <= 0) {
#ifndef _WIN32
            if(nw<0 && errno == EINTR) continue;
#else
            if(nw == 0) return -1;
#endif
            else {
                err_msg("Write error");
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