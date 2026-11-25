
#ifndef MODEL_H
#define MODEL_H

#define MIN_PRIORITY 0
#define MAX_PRIORITY 255
#define MAX_MESSAGE_LENGTH 256

typedef struct {
    int id;
    int priority;
    char text[MAX_MESSAGE_LENGTH];
} Message;

typedef struct QueueNode {
    Message message;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *head;
} PriorityQueue;

void queue_init(PriorityQueue *queue);

int queue_is_empty(const PriorityQueue *queue);

int queue_enqueue(PriorityQueue *queue, Message message);

int queue_dequeue_first(PriorityQueue *queue, Message *result);

int queue_dequeue_priority(
    PriorityQueue *queue,
    int priority,
    Message *result
);

int queue_dequeue_min_priority(
    PriorityQueue *queue,
    int min_priority,
    Message *result
);

void queue_destroy(PriorityQueue *queue);

#endif

