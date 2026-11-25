
#include "model.h"

#include <stdlib.h>

void queue_init(PriorityQueue *queue) {
    if (queue != NULL) {
        queue->head = NULL;
    }
}

int queue_is_empty(const PriorityQueue *queue) {
    return queue == NULL || queue->head == NULL;
}

static int queue_contains_id(const PriorityQueue *queue, int id) {
    QueueNode *current = queue->head;

    while (current != NULL) {
        if (current->message.id == id) {
            return 1;
        }

        current = current->next;
    }

    return 0;
}

int queue_enqueue(PriorityQueue *queue, Message message) {
    if (queue == NULL ||
        message.priority < MIN_PRIORITY ||
        message.priority > MAX_PRIORITY) {
        return 0;
    }

    if (queue_contains_id(queue, message.id)) {
        return 0;
    }

    QueueNode *new_node = malloc(sizeof(QueueNode));

    if (new_node == NULL) {
        return 0;
    }

    new_node->message = message;
    new_node->next = NULL;

    if (queue->head == NULL ||
        message.priority > queue->head->message.priority) {

        new_node->next = queue->head;
        queue->head = new_node;

        return 1;
    }

    QueueNode *current = queue->head;

    while (current->next != NULL &&
           current->next->message.priority >= message.priority) {

        current = current->next;
    }

    new_node->next = current->next;
    current->next = new_node;

    return 1;
}

int queue_dequeue_first(PriorityQueue *queue, Message *result) {
    if (queue_is_empty(queue) || result == NULL) {
        return 0;
    }

    QueueNode *temp = queue->head;

    *result = temp->message;

    queue->head = temp->next;

    free(temp);

    return 1;
}

int queue_dequeue_priority(
    PriorityQueue *queue,
    int priority,
    Message *result
) {
    if (queue == NULL || result == NULL ||
        priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        return 0;
    }

    QueueNode *current = queue->head;
    QueueNode *previous = NULL;

    while (current != NULL) {

        if (current->message.priority == priority) {

            *result = current->message;

            if (previous == NULL) {
                queue->head = current->next;
            } else {
                previous->next = current->next;
            }

            free(current);

            return 1;
        }

        previous = current;
        current = current->next;
    }

    return 0;
}

int queue_dequeue_min_priority(
    PriorityQueue *queue,
    int min_priority,
    Message *result
) {
    if (queue == NULL || result == NULL ||
        min_priority < MIN_PRIORITY ||
        min_priority > MAX_PRIORITY) {
        return 0;
    }

    QueueNode *current = queue->head;
    QueueNode *previous = NULL;

    while (current != NULL) {

        if (current->message.priority >= min_priority) {

            *result = current->message;

            if (previous == NULL) {
                queue->head = current->next;
            } else {
                previous->next = current->next;
            }

            free(current);

            return 1;
        }

        previous = current;
        current = current->next;
    }

    return 0;
}

void queue_destroy(PriorityQueue *queue) {
    if (queue == NULL) {
        return;
    }

    QueueNode *current = queue->head;

    while (current != NULL) {
        QueueNode *temp = current;
        current = current->next;
        free(temp);
    }

    queue->head = NULL;
}

