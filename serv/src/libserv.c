#include "libserv.h"

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
    if(close(sockfd) == -1) {
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
    if((n = poll(fdarr, nfds, -1)) < 0) 
        err_sys("poll error");
    return n;
}

void Write(int sockfd, const void *vptr, size_t n) {
    size_t rem;
    ssize_t nw;
    const char *ptr = vptr;
    rem = n;
    while(rem>0) {
        if((nw = write(sockfd, ptr, rem)) <= 0) {
            if(nw<0 && errno == EINTR) nw = 0;
            else {
                err_sys("Write error");
                return;
            }
        }
        rem -= nw;
        ptr += nw;
    }
    return;
}

void err_msg(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    return;
}

void err_quit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void err_sys(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap); 
    fprintf(stderr, ": %s\n", strerror(errno)); 
    va_end(ap);
    exit(1);
}