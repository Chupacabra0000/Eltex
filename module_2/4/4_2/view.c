
#include "view.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void view_show_menu(void) {
    printf("\n========== PRIORITY QUEUE ==========\n");
    printf("1. Generate message\n");
    printf("2. Display queue\n");
    printf("3. Extract first message\n");
    printf("4. Extract message with exact priority\n");
    printf("5. Extract message with priority >= specified\n");
    printf("0. Exit\n");
    printf("====================================\n");
}

int view_read_integer(const char *prompt, int *value) {
    char buffer[64];
    char extra;

    printf("%s", prompt);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    if (sscanf(buffer, " %d %c", value, &extra) != 1) {
        return 0;
    }

    return 1;
}

int view_read_message(Message *message) {
    if (message == NULL) {
        return 0;
    }

    if (!view_read_integer("Enter message ID: ", &message->id)) {
        return 0;
    }

    if (!view_read_integer("Enter priority (0-255): ",
                          &message->priority)) {
        return 0;
    }

    if (message->priority < MIN_PRIORITY ||
        message->priority > MAX_PRIORITY) {
        return 0;
    }

    printf("Enter message text: ");

    if (fgets(message->text, MAX_MESSAGE_LENGTH, stdin) == NULL) {
        return 0;
    }

    message->text[strcspn(message->text, "\n")] = '\0';

    return 1;
}

void view_show_message(const Message *message) {
    if (message == NULL) {
        return;
    }

    printf("\n---------- MESSAGE ----------\n");
    printf("ID: %d\n", message->id);
    printf("Priority: %d\n", message->priority);
    printf("Text: %s\n", message->text);
    printf("-----------------------------\n");
}

void view_show_queue(const PriorityQueue *queue) {
    if (queue == NULL || queue_is_empty(queue)) {
        printf("\nQueue is empty.\n");
        return;
    }

    QueueNode *current = queue->head;

    printf("\n========== QUEUE CONTENTS ==========\n");

    while (current != NULL) {
        view_show_message(&current->message);
        current = current->next;
    }

    printf("====================================\n");
}

void view_show_error(const char *message) {
    printf("\nError: %s\n", message);
}

