#ifndef MODEL_H
#define MODEL_H

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

    struct Node *left;
    struct Node *right;

    int height;
} Node;

typedef struct {
    Node *root;
    int count;
} Manager;

/* Fields that can be edited */
enum Field {
    FIELD_END = 0,
    FIELD_SURNAME,
    FIELD_FIRST_NAME,
    FIELD_WORKPLACE,
    FIELD_JOB_TITLE,
    FIELD_PHONE,
    FIELD_EMAIL,
    FIELD_SOCIAL_MEDIA,
    FIELD_MESSENGER
};

/* Traversal orders */
typedef enum {
    IN_ORDER,
    PRE_ORDER,
    POST_ORDER
} TraversalOrder;

/* Function called for each contact during traversal */
typedef void (*ContactVisitor)(const Person *person, int index, void *context);

/* Basic operations */
void initManager(Manager *manager);

int isEmpty(const Manager *manager);

int addContact(Manager *manager, const Person *person);

int editContact(Manager *manager, int index, ...);

int deleteContact(Manager *manager, int index);

const Person *getContact(const Manager *manager, int index);

/* Traversal */
void traverseContacts(
    const Manager *manager,
    TraversalOrder order,
    ContactVisitor visitor,
    void *context
);

/* Memory management */
void freeTree(Node *root);

#endif