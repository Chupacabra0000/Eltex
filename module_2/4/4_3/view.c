#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================================================
   INPUT HELPERS
   ===================================================== */

static void readLine(const char *message, char *buffer, int size)
{
    printf("%s", message);

    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
}

static int readInt(const char *message)
{
    char buffer[50];
    char *end;
    long value;

    while (1) {
        readLine(message, buffer, sizeof(buffer));

        value = strtol(buffer, &end, 10);

        if (end != buffer && *end == '\0')
            return (int)value;

        printf("Invalid number. Try again.\n");
    }
}

/* =====================================================
   DISPLAY ONE CONTACT
   ===================================================== */

static void showContact(const Person *person, int index)
{
    printf("\n-----------------------------\n");
    printf("Contact #%d\n", index + 1);
    printf("-----------------------------\n");

    printf("Surname:      %s\n", person->surname);
    printf("First name:   %s\n", person->firstName);
    printf("Workplace:    %s\n", person->workplace);
    printf("Job title:    %s\n", person->jobTitle);
    printf("Phone:        %s\n", person->phone);
    printf("Email:        %s\n", person->email);
    printf("Social media: %s\n", person->socialMedia);
    printf("Messenger:    %s\n", person->messenger);

    printf("-----------------------------\n");
}

/* =====================================================
   TRAVERSAL CALLBACKS
   ===================================================== */

static void displayContactCallback(
    const Person *person,
    int index,
    void *context
)
{
    (void)context;

    showContact(person, index);
}

static void displayIndexCallback(
    const Person *person,
    int traversalIndex,
    void *context
)
{
    const Manager *manager = context;
    int i;

    (void)traversalIndex;

    for (i = 0; i < manager->count; i++) {

        if (getContact(manager, i) == person) {
            printf("%d ", i + 1);
            return;
        }
    }
}

/* =====================================================
   DISPLAY ALL CONTACTS
   In-order traversal: sorted by surname and first name.
   ===================================================== */

static void listContactsView(const Manager *manager)
{
    if (isEmpty(manager)) {
        printf("\nThe phonebook is empty.\n");
        return;
    }

    printf("\n========== CONTACTS ==========\n");

    traverseContacts(
        manager,
        IN_ORDER,
        displayContactCallback,
        NULL
    );
}

/* =====================================================
   PRINT TREE INDEXES ONLY
   ===================================================== */

static void printIndexesView(
    const Manager *manager,
    TraversalOrder order
)
{
    if (isEmpty(manager)) {
        printf("\nThe tree is empty.\n");
        return;
    }

    traverseContacts(
        manager,
        order,
        displayIndexCallback,
        (void *)manager
    );

    printf("\n");
}

/* =====================================================
   PRINT TREE STRUCTURE
   ===================================================== */

#define TREE_SPACING 6

static void printTreeRecursive(const Node *root, int level)
{
    int i;

    if (root == NULL)
        return;

    /* Print right subtree first */
    printTreeRecursive(root->right, level + 1);

    /* Indentation */
    for (i = 0; i < level * TREE_SPACING; i++)
        printf(" ");

    /* Print node */
    printf("%s %s\n",
           root->person.surname,
           root->person.firstName);

    /* Print left subtree */
    printTreeRecursive(root->left, level + 1);
}

static void printTreeView(const Manager *manager)
{
    if (isEmpty(manager)) {
        printf("\nThe tree is empty.\n");
        return;
    }

    printf("\n========== AVL TREE ==========\n\n");

    printTreeRecursive(manager->root, 0);

    printf("\n");
}


/* =====================================================
   ADD CONTACT
   ===================================================== */

static void addContactView(Manager *manager)
{
    Person person = {0};

    printf("\n========== ADD CONTACT ==========\n");

    readLine("Surname: ", person.surname, MAX_TEXT);
    readLine("First name: ", person.firstName, MAX_TEXT);

    if (person.surname[0] == '\0' ||
        person.firstName[0] == '\0') {
        printf("Surname and first name are required.\n");
        return;
    }

    readLine("Workplace: ", person.workplace, MAX_TEXT);
    readLine("Job title: ", person.jobTitle, MAX_TEXT);
    readLine("Phone: ", person.phone, MAX_TEXT);
    readLine("Email: ", person.email, MAX_TEXT);
    readLine("Social media: ", person.socialMedia, MAX_TEXT);
    readLine("Messenger: ", person.messenger, MAX_TEXT);

    if (addContact(manager, &person)) {
        printf("\nContact added successfully.\n");
    } else {
        printf("\nCould not add contact.\n");
        printf("A contact with this surname and first name may already exist,\n");
        printf("or memory allocation may have failed.\n");
    }
}

/* =====================================================
   PRINT CONTACT BY INDEX
   User enters a one-based index.
   ===================================================== */

static void printContactByIndexView(const Manager *manager)
{
    int index;
    const Person *person;

    if (isEmpty(manager)) {
        printf("\nThe phonebook is empty.\n");
        return;
    }

    index = readInt("Enter contact number: ");

    if (index < 1 || index > manager->count) {
        printf("Invalid contact number.\n");
        return;
    }

    person = getContact(manager, index - 1);

    if (person != NULL)
        showContact(person, index - 1);
    else
        printf("Contact not found.\n");
}

/* =====================================================
   EDIT CONTACT
   Empty input means "do not change this field".
   ===================================================== */

static void editContactView(Manager *manager)
{
    int index;

    char surname[MAX_TEXT] = "";
    char firstName[MAX_TEXT] = "";
    char workplace[MAX_TEXT] = "";
    char jobTitle[MAX_TEXT] = "";
    char phone[MAX_TEXT] = "";
    char email[MAX_TEXT] = "";
    char socialMedia[MAX_TEXT] = "";
    char messenger[MAX_TEXT] = "";

    if (isEmpty(manager)) {
        printf("\nThe phonebook is empty.\n");
        return;
    }

    listContactsView(manager);

    index = readInt("\nEnter contact number to edit: ");

    if (index < 1 || index > manager->count) {
        printf("Invalid contact number.\n");
        return;
    }

    printf("\nLeave a field empty to keep its current value.\n");

    readLine("New surname: ", surname, MAX_TEXT);
    readLine("New first name: ", firstName, MAX_TEXT);
    readLine("New workplace: ", workplace, MAX_TEXT);
    readLine("New job title: ", jobTitle, MAX_TEXT);
    readLine("New phone: ", phone, MAX_TEXT);
    readLine("New email: ", email, MAX_TEXT);
    readLine("New social media: ", socialMedia, MAX_TEXT);
    readLine("New messenger: ", messenger, MAX_TEXT);

    if (editContact(
            manager,
            index - 1,

            FIELD_SURNAME, surname,
            FIELD_FIRST_NAME, firstName,
            FIELD_WORKPLACE, workplace,
            FIELD_JOB_TITLE, jobTitle,
            FIELD_PHONE, phone,
            FIELD_EMAIL, email,
            FIELD_SOCIAL_MEDIA, socialMedia,
            FIELD_MESSENGER, messenger,

            FIELD_END
        )) {

        printf("\nContact updated successfully.\n");

    } else {

        printf("\nCould not update contact.\n");
        printf("The new name may already belong to another contact.\n");
    }
}

/* =====================================================
   DELETE CONTACT
   ===================================================== */

static void deleteContactView(Manager *manager)
{
    int index;

    if (isEmpty(manager)) {
        printf("\nThe phonebook is empty.\n");
        return;
    }

    listContactsView(manager);

    index = readInt("\nEnter contact number to delete: ");

    if (index < 1 || index > manager->count) {
        printf("Invalid contact number.\n");
        return;
    }

    if (deleteContact(manager, index - 1)) {
        printf("\nContact deleted successfully.\n");
    } else {
        printf("\nCould not delete contact.\n");
    }
}

/* =====================================================
   TRAVERSAL MENU
   ===================================================== */

static void traversalMenu(const Manager *manager)
{
    int choice;

    printf("\n========== TREE TRAVERSAL ==========\n");
    printf("1. In-order\n");
    printf("2. Pre-order\n");
    printf("3. Post-order\n");

    choice = readInt("Choose traversal: ");

    switch (choice) {

        case 1:
            printf("\nIn-order indexes: ");
            printIndexesView(manager, IN_ORDER);
            break;

        case 2:
            printf("\nPre-order indexes: ");
            printIndexesView(manager, PRE_ORDER);
            break;

        case 3:
            printf("\nPost-order indexes: ");
            printIndexesView(manager, POST_ORDER);
            break;

        default:
            printf("Invalid choice.\n");
    }
}

/* =====================================================
   MAIN MENU
   ===================================================== */

void runView(Manager *manager)
{
    int choice;

    do {
        printf("\n========== PHONEBOOK ==========\n");

        printf("1. Add contact\n");
        printf("2. List all contacts\n");
        printf("3. Print contact by index\n");
        printf("4. Edit contact\n");
        printf("5. Delete contact\n");
        printf("6. Print tree indexes\n");
        printf("7. Check if tree is empty\n");
        printf("8. Print tree structure\n");
        printf("0. Exit\n");

        choice = readInt("\nChoose an option: ");

        switch (choice) {

            case 1:
                addContactView(manager);
                break;

            case 2:
                listContactsView(manager);
                break;

            case 3:
                printContactByIndexView(manager);
                break;

            case 4:
                editContactView(manager);
                break;

            case 5:
                deleteContactView(manager);
                break;

            case 6:
                traversalMenu(manager);
                break;

            case 7:
                if (isEmpty(manager))
                    printf("\nThe tree is empty.\n");
                else
                    printf("\nThe tree contains %d contacts.\n",
                           manager->count);
                break;
                
            case 8:
                printTreeView(manager);
                break;

            case 0:
                printf("\nExiting phonebook...\n");
                break;

            default:
                printf("\nInvalid option.\n");
        }

    } while (choice != 0);
}