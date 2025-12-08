#include "libserv.h"

#define DEBUG

/**** Queue functions ****/

void init_Queue(Queue* q) {
    q->dm_head = calloc(1, sizeof(Node));
    q->dm_head->nxt = NULL;
    q->tail = q->dm_head;
}

void Delete(Queue* q, int idx) {
    Node* tmp = q->dm_head;
    while(tmp->nxt != NULL) {
        if(tmp->nxt->i == idx) {
            Node* d = tmp->nxt;
            tmp->nxt = d->nxt;
            if(q->tail == d) q->tail = tmp;
            free(d);
#ifdef DEBUG
            printf("Deleted %d\n", idx);
#endif
            return;
        }
        tmp = tmp->nxt;
    }
}

bool isEmpty(Queue* q) {
    return (q->dm_head->nxt == NULL);
}

void pop(Queue* q) {
    if(isEmpty(q)) return; 
    Node* tmp = q->dm_head->nxt;
    q->dm_head->nxt = tmp->nxt;
    if(q->tail == tmp) q->tail = q->dm_head;
    free(tmp);
#ifdef DEBUG
    printf("Pop\n");
#endif
}

void push(Queue* q, int sockfd, int roomID, int i) {
    Delete(q, i);
    Node* newNode = malloc(sizeof(Node));
    newNode->sockfd = sockfd;
    newNode->roomID = roomID;
    newNode->i = i;
    newNode->nxt = NULL;
    q->tail->nxt = newNode;
    q->tail = newNode;
#ifdef DEBUG
    printf("Pushed %d\n", i);
#endif
}

Node* front(Queue* q) {
    if(isEmpty(q)) return NULL;
    return q->dm_head->nxt;
}