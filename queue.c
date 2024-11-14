#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN_MSG 1024
#define MAX_MSG_IN_QUEUE 6

typedef struct {
    char messages[MAX_MSG_IN_QUEUE][MAX_LEN_MSG];
    int max_message;
    int head;
    int tail;
} Queue;

void init_messages_queue(Queue* queue) {
    queue->max_message = MAX_MSG_IN_QUEUE;
    queue->head = -1;
    queue->tail = 0;
}

int queue_is_empty(Queue* queue) {
    return queue->head == -1; // La queue est vide si head est -1
}

int queue_is_full(Queue* queue) {
    // La queue est pleine si la prochaine position de tail est head
    return (queue->tail + 1) % queue->max_message == queue->head;
}

void put_in_queue(Queue* queue, const char *message) {
    if (queue_is_full(queue)) {
        fprintf(stderr, "ERROR: put_in_queue: the queue is full\n");
        return;
    }

    snprintf(queue->messages[queue->tail], sizeof(queue->messages[queue->tail]), "%s", message);
    
    if (queue->head == -1) {
        queue->head = 0;
    }

    queue->tail = (queue->tail + 1) % queue->max_message;
}

char* pop_head(Queue* queue) {
    if (queue_is_empty(queue)) {
        fprintf(stderr, "ERROR: pop_head: the queue is empty\n");
        return NULL;
    }

    char* head_message = queue->messages[queue->head];    
    if (queue->head == queue->tail - 1) {
        queue->head = -1;
        queue->tail = 0;
    } else {
        queue->head = (queue->head + 1) % queue->max_message;
    }

    return head_message;
}

void print_queue(Queue* queue) {
    if (queue_is_empty(queue)) {
        printf("La queue est vide.\n");
        return;
    }

    int i = queue->head;
    printf("Contenu de la queue :\n");

    while (1) {
        printf("Index = %d, Contenu : %s\n", i, queue->messages[i]);
        i = (i + 1) % queue->max_message;
        if (i == queue->tail) {
            break;
        }
    }
}
