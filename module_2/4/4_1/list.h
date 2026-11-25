#ifndef LIST_H
#define LIST_H

#define MAX_TEXT 100

typedef struct {
    char surname[MAX_TEXT];
    char firstName[MAX_TEXT];
    char workplace[MAX_TEXT];
    char jobTitle[MAX_TEXT];
    char phone[MAX_TEXT];
    char email[MAX_TEXT];
    char socialMedia[MAX_TEXT];
    char messenger[MAX_TEXT];
} Person;

typedef struct Node {
    Person person;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    int count;
} List;

void initList(List *list);

int insertSorted(List *list, const Person *person);

void removeNode(List *list, Node *node);

Node *getNode(List *list, int index);

const Node *getNodeConst(const List *list, int index);

void repositionNode(List *list, Node *node);

void destroyList(List *list);

#endif