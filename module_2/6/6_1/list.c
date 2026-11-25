#include "list.h"

#include <stdlib.h>
#include <string.h>

void initList(List *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

int insertSorted(List *list, const Person *person)
{
    Node *newNode = malloc(sizeof(Node));

    if (newNode == NULL) {
        return 0;
    }

    newNode->person = *person;
    newNode->prev = NULL;
    newNode->next = NULL;

    Node *current = list->head;

    /* Find the first surname that is greater than the new surname. */
    while (current != NULL &&
           strcmp(current->person.surname, person->surname) <= 0) {
        current = current->next;
    }

    /* Insert at the end. */
    if (current == NULL) {
        newNode->prev = list->tail;

        if (list->tail != NULL) {
            list->tail->next = newNode;
        } else {
            list->head = newNode;
        }

        list->tail = newNode;
    }

    /* Insert at the beginning or in the middle. */
    else {
        newNode->next = current;
        newNode->prev = current->prev;

        if (current->prev != NULL) {
            current->prev->next = newNode;
        } else {
            list->head = newNode;
        }

        current->prev = newNode;
    }

    list->count++;

    return 1;
}

void removeNode(List *list, Node *node)
{
    if (list == NULL || node == NULL) {
        return;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    free(node);
    list->count--;
}

Node *getNode(List *list, int index)
{
    if (list == NULL || index < 0 || index >= list->count) {
        return NULL;
    }

    Node *current;

    /* Start from whichever end is closer. */
    if (index < list->count / 2) {
        current = list->head;

        for (int i = 0; i < index; i++) {
            current = current->next;
        }
    } else {
        current = list->tail;

        for (int i = list->count - 1; i > index; i--) {
            current = current->prev;
        }
    }

    return current;
}

const Node *getNodeConst(const List *list, int index)
{
    if (list == NULL || index < 0 || index >= list->count) {
        return NULL;
    }

    const Node *current;

    if (index < list->count / 2) {
        current = list->head;

        for (int i = 0; i < index; i++) {
            current = current->next;
        }
    } else {
        current = list->tail;

        for (int i = list->count - 1; i > index; i--) {
            current = current->prev;
        }
    }

    return current;
}

void repositionNode(List *list, Node *node)
{
    if (list == NULL || node == NULL) {
        return;
    }

    /* Detach the node without freeing it. */
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    node->prev = NULL;
    node->next = NULL;

    /* Find the new position by surname. */
    Node *current = list->head;

    while (current != NULL &&
           strcmp(current->person.surname,
                  node->person.surname) <= 0) {
        current = current->next;
    }

    /* Insert at the end. */
    if (current == NULL) {
        node->prev = list->tail;

        if (list->tail != NULL) {
            list->tail->next = node;
        } else {
            list->head = node;
        }

        list->tail = node;
    }

    /* Insert at the beginning or in the middle. */
    else {
        node->next = current;
        node->prev = current->prev;

        if (current->prev != NULL) {
            current->prev->next = node;
        } else {
            list->head = node;
        }

        current->prev = node;
    }
}

void destroyList(List *list)
{
    if (list == NULL) {
        return;
    }

    Node *current = list->head;

    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }

    initList(list);
}