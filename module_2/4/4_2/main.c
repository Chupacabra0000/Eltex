
#include "model.h"
#include "view.h"

#include <stdio.h>

static void generate_test_messages(PriorityQueue *queue) {
    Message messages[] = {
        {1, 120, "Hello"},
        {2, 255, "Emergency!"},
        {3, 80, "Normal message"},
        {4, 200, "Important message"},
        {5, 120, "Second message"},
        {6, 50, "Low priority"},
        {7, 180, "Urgent message"},
        {8, 200, "Another important message"}
    };

    int count = sizeof(messages) / sizeof(messages[0]);

    printf("\nGenerating test messages...\n");

    for (int i = 0; i < count; i++) {
        if (queue_enqueue(queue, messages[i])) {
            printf("Generated ID=%d, Priority=%d\n",
                   messages[i].id,
                   messages[i].priority);
        } else {
            view_show_error("Could not generate message.");
        }
    }
}

int main(void) {
    PriorityQueue queue;
    Message result;

    int choice;
    int priority;

    queue_init(&queue);

    // Simulate message generation
    generate_test_messages(&queue);

    while (1) {
        view_show_menu();

        if (!view_read_integer("Choose operation: ", &choice)) {
            view_show_error("Invalid input.");
            continue;
        }

        switch (choice) {

            case 1: {
                Message message;

                if (!view_read_message(&message)) {
                    view_show_error("Invalid message data.");
                    break;
                }

                if (queue_enqueue(&queue, message)) {
                    printf("Message added successfully.\n");
                } else {
                    view_show_error("Failed to add message.");
                }

                break;
            }

            case 2:
                view_show_queue(&queue);
                break;

            case 3:
                if (queue_dequeue_first(&queue, &result)) {
                    view_show_message(&result);
                } else {
                    view_show_error("Queue is empty.");
                }
                break;

            case 4:
                if (!view_read_integer(
                        "Enter exact priority: ", &priority)) {
                    view_show_error("Invalid priority.");
                    break;
                }

                if (queue_dequeue_priority(
                        &queue, priority, &result)) {
                    view_show_message(&result);
                } else {
                    view_show_error("Message not found.");
                }

                break;

            case 5:
                if (!view_read_integer(
                        "Enter minimum priority: ", &priority)) {
                    view_show_error("Invalid priority.");
                    break;
                }

                if (queue_dequeue_min_priority(
                        &queue, priority, &result)) {
                    view_show_message(&result);
                } else {
                    view_show_error("Message not found.");
                }

                break;

            case 0:
                queue_destroy(&queue);
                printf("Program finished.\n");
                return 0;

            default:
                view_show_error("Unknown operation.");
        }
    }
}

