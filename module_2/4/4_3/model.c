#include "model.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* =====================================================
   STRING HELPER
   ===================================================== */

static void copyText(char *destination, const char *source)
{
    int i = 0;

    while (i < MAX_TEXT - 1 && source[i] != '\0') {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

/* =====================================================
   COMPARISON
   Sort by surname, then first name.
   ===================================================== */

static int comparePersons(const Person *a, const Person *b)
{
    int result = strcmp(a->surname, b->surname);

    if (result == 0)
        result = strcmp(a->firstName, b->firstName);

    return result;
}

/* =====================================================
   AVL HEIGHT AND BALANCE
   ===================================================== */

static int height(const Node *node)
{
    return node == NULL ? 0 : node->height;
}

static int max(int a, int b)
{
    return a > b ? a : b;
}

static void updateHeight(Node *node)
{
    node->height = 1 + max(
        height(node->left),
        height(node->right)
    );
}

static int getBalance(const Node *node)
{
    if (node == NULL)
        return 0;

    return height(node->left) - height(node->right);
}

/* =====================================================
   AVL ROTATIONS
   ===================================================== */

static Node *rotateRight(Node *z)
{
    Node *y = z->left;
    Node *t2 = y->right;

    y->right = z;
    z->left = t2;

    updateHeight(z);
    updateHeight(y);

    return y;
}

static Node *rotateLeft(Node *z)
{
    Node *y = z->right;
    Node *t2 = y->left;

    y->left = z;
    z->right = t2;

    updateHeight(z);
    updateHeight(y);

    return y;
}

/* =====================================================
   AVL BALANCING
   ===================================================== */

static Node *balanceNode(Node *node)
{
    int balance;

    if (node == NULL)
        return NULL;

    updateHeight(node);

    balance = getBalance(node);

    /* Left-heavy tree */
    if (balance > 1) {

        /* Left-right case */
        if (getBalance(node->left) < 0)
            node->left = rotateLeft(node->left);

        return rotateRight(node);
    }

    /* Right-heavy tree */
    if (balance < -1) {

        /* Right-left case */
        if (getBalance(node->right) > 0)
            node->right = rotateRight(node->right);

        return rotateLeft(node);
    }

    return node;
}

/* =====================================================
   NODE CREATION
   ===================================================== */

static Node *createNode(const Person *person)
{
    Node *node = malloc(sizeof(Node));

    if (node == NULL)
        return NULL;

    node->person = *person;

    node->left = NULL;
    node->right = NULL;

    node->height = 1;

    return node;
}

/* =====================================================
   SEARCH BY PERSON KEY
   ===================================================== */

static Node *findNode(Node *root, const Person *person)
{
    int result;

    while (root != NULL) {

        result = comparePersons(person, &root->person);

        if (result == 0)
            return root;

        if (result < 0)
            root = root->left;
        else
            root = root->right;
    }

    return NULL;
}

/* =====================================================
   INSERTION
   ===================================================== */

static Node *insertNode(Node *root, Node *newNode)
{
    int result;

    if (root == NULL)
        return newNode;

    result = comparePersons(&newNode->person, &root->person);

    if (result < 0)
        root->left = insertNode(root->left, newNode);
    else
        root->right = insertNode(root->right, newNode);

    return balanceNode(root);
}

/* =====================================================
   INITIALIZATION
   ===================================================== */

void initManager(Manager *manager)
{
    if (manager == NULL)
        return;

    manager->root = NULL;
    manager->count = 0;
}

/* =====================================================
   CHECK IF TREE IS EMPTY
   ===================================================== */

int isEmpty(const Manager *manager)
{
    return manager == NULL || manager->root == NULL;
}

/* =====================================================
   ADD CONTACT
   ===================================================== */

int addContact(Manager *manager, const Person *person)
{
    Node *node;

    if (manager == NULL || person == NULL)
        return 0;

    /* Surname and first name are mandatory */
    if (person->surname[0] == '\0' ||
        person->firstName[0] == '\0')
        return 0;

    /* Reject identical surname + first name */
    if (findNode(manager->root, person) != NULL)
        return 0;

    node = createNode(person);

    if (node == NULL)
        return 0;

    manager->root = insertNode(manager->root, node);

    manager->count++;

    return 1;
}

/* =====================================================
   FIND MINIMUM NODE
   ===================================================== */

static Node *findMin(Node *root)
{
    while (root != NULL && root->left != NULL)
        root = root->left;

    return root;
}

/* =====================================================
   DELETE NODE
   ===================================================== */

static Node *deleteNode(Node *root, const Person *person)
{
    int result;

    if (root == NULL)
        return NULL;

    result = comparePersons(person, &root->person);

    if (result < 0) {

        root->left = deleteNode(root->left, person);

    } else if (result > 0) {

        root->right = deleteNode(root->right, person);

    } else {

        /* No left child */
        if (root->left == NULL) {
            Node *temp = root->right;

            free(root);

            return temp;
        }

        /* No right child */
        if (root->right == NULL) {
            Node *temp = root->left;

            free(root);

            return temp;
        }

        /* Two children */
        {
            Node *successor = findMin(root->right);

            root->person = successor->person;

            root->right = deleteNode(
                root->right,
                &successor->person
            );
        }
    }

    return balanceNode(root);
}

/* =====================================================
   GET CONTACT BY SORTED INDEX
   Index is zero-based.
   ===================================================== */

static const Node *getNodeByIndex(
    const Node *root,
    int index,
    int *current
)
{
    const Node *result;

    if (root == NULL)
        return NULL;

    result = getNodeByIndex(root->left, index, current);

    if (result != NULL)
        return result;

    if (*current == index)
        return root;

    (*current)++;

    return getNodeByIndex(root->right, index, current);
}

const Person *getContact(const Manager *manager, int index)
{
    int current = 0;
    const Node *node;

    if (manager == NULL || index < 0 || index >= manager->count)
        return NULL;

    node = getNodeByIndex(manager->root, index, &current);

    return node == NULL ? NULL : &node->person;
}

/* =====================================================
   DELETE CONTACT BY SORTED INDEX
   ===================================================== */

int deleteContact(Manager *manager, int index)
{
    const Person *found;
    Person person;

    if (manager == NULL)
        return 0;

    found = getContact(manager, index);

    if (found == NULL)
        return 0;

    /* Copy before deleting the node */
    person = *found;

    manager->root = deleteNode(manager->root, &person);

    manager->count--;

    return 1;
}

/* =====================================================
   EDIT CONTACT
   Usage:
   editContact(manager, index,
               FIELD_PHONE, "12345",
               FIELD_EMAIL, "mail@example.com",
               FIELD_END);
   ===================================================== */

int editContact(Manager *manager, int index, ...)
{
    const Person *found;

    Person oldPerson;
    Person updatedPerson;

    va_list args;

    int field;
    char *value;

    int nameChanged = 0;
    int changed = 0;

    Node *node;
    Node *newNode;

    if (manager == NULL)
        return 0;

    found = getContact(manager, index);

    if (found == NULL)
        return 0;

    oldPerson = *found;
    updatedPerson = oldPerson;

    va_start(args, index);

    while ((field = va_arg(args, int)) != FIELD_END) {

        value = va_arg(args, char *);

        if (value == NULL || value[0] == '\0')
            continue;

        switch (field) {

            case FIELD_SURNAME:
                copyText(updatedPerson.surname, value);
                nameChanged = 1;
                changed = 1;
                break;

            case FIELD_FIRST_NAME:
                copyText(updatedPerson.firstName, value);
                nameChanged = 1;
                changed = 1;
                break;

            case FIELD_WORKPLACE:
                copyText(updatedPerson.workplace, value);
                changed = 1;
                break;

            case FIELD_JOB_TITLE:
                copyText(updatedPerson.jobTitle, value);
                changed = 1;
                break;

            case FIELD_PHONE:
                copyText(updatedPerson.phone, value);
                changed = 1;
                break;

            case FIELD_EMAIL:
                copyText(updatedPerson.email, value);
                changed = 1;
                break;

            case FIELD_SOCIAL_MEDIA:
                copyText(updatedPerson.socialMedia, value);
                changed = 1;
                break;

            case FIELD_MESSENGER:
                copyText(updatedPerson.messenger, value);
                changed = 1;
                break;

            default:
                va_end(args);
                return 0;
        }
    }

    va_end(args);

    if (!changed)
        return 1;

    /* Mandatory fields cannot become empty */
    if (updatedPerson.surname[0] == '\0' ||
        updatedPerson.firstName[0] == '\0')
        return 0;

    /*
       If the name key did not actually change,
       update the existing node directly.
    */
    if (comparePersons(&oldPerson, &updatedPerson) == 0) {

        node = findNode(manager->root, &oldPerson);

        if (node == NULL)
            return 0;

        node->person = updatedPerson;

        return 1;
    }

    /*
       If the new surname + first name already exists,
       reject the update.
    */
    if (findNode(manager->root, &updatedPerson) != NULL)
        return 0;

    /*
       Allocate the replacement before deleting the old node.
       This prevents data loss if malloc fails.
    */
    newNode = createNode(&updatedPerson);

    if (newNode == NULL)
        return 0;

    manager->root = deleteNode(manager->root, &oldPerson);

    manager->root = insertNode(manager->root, newNode);

    /*
       Contact count does not change.
       AVL balancing happens during deletion and insertion.
    */

    (void)nameChanged;

    return 1;
}

/* =====================================================
   TRAVERSAL HELPERS
   ===================================================== */

static void inOrder(
    const Node *root,
    ContactVisitor visitor,
    void *context,
    int *index
)
{
    if (root == NULL)
        return;

    inOrder(root->left, visitor, context, index);

    visitor(&root->person, (*index)++, context);

    inOrder(root->right, visitor, context, index);
}

static void preOrder(
    const Node *root,
    ContactVisitor visitor,
    void *context,
    int *index
)
{
    if (root == NULL)
        return;

    visitor(&root->person, (*index)++, context);

    preOrder(root->left, visitor, context, index);
    preOrder(root->right, visitor, context, index);
}

static void postOrder(
    const Node *root,
    ContactVisitor visitor,
    void *context,
    int *index
)
{
    if (root == NULL)
        return;

    postOrder(root->left, visitor, context, index);
    postOrder(root->right, visitor, context, index);

    visitor(&root->person, (*index)++, context);
}

/* =====================================================
   PUBLIC TRAVERSAL FUNCTION
   ===================================================== */

void traverseContacts(
    const Manager *manager,
    TraversalOrder order,
    ContactVisitor visitor,
    void *context
)
{
    int index = 0;

    if (manager == NULL || visitor == NULL)
        return;

    switch (order) {

        case IN_ORDER:
            inOrder(manager->root, visitor, context, &index);
            break;

        case PRE_ORDER:
            preOrder(manager->root, visitor, context, &index);
            break;

        case POST_ORDER:
            postOrder(manager->root, visitor, context, &index);
            break;
    }
}

/* =====================================================
   FREE ENTIRE TREE
   ===================================================== */

void freeTree(Node *root)
{
    if (root == NULL)
        return;

    freeTree(root->left);
    freeTree(root->right);

    free(root);
}