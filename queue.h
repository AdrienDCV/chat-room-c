#ifndef QUEUE_H
#define QUEUE_H

#define MAX_LEN_MSG 1024
#define MAX_MSG_IN_QUEUE 5

typedef struct {
    char messages[MAX_MSG_IN_QUEUE][MAX_LEN_MSG];
    int max_message;
    int head;
    int tail;
} Queue;

void init_messages_queue(Queue* queue);
int queue_is_empty(Queue* queue);
int queue_is_full(Queue* queue);
void put_in_queue(Queue* queue, const char *message);
char* pop_head(Queue* queue);
void print_queue(Queue* queue);

#endif
