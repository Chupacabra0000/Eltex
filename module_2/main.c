
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CONTACTS 100
#define NAME_SIZE 100
#define FIELD_SIZE 150

typedef struct Person {
    char lastName[NAME_SIZE];
    char firstName[NAME_SIZE];
    char middleName[NAME_SIZE];
    char workplace[FIELD_SIZE];
    char position[FIELD_SIZE];
    char phone[FIELD_SIZE];
    char email[FIELD_SIZE];
    char socialNetwork[FIELD_SIZE];
    char messenger[FIELD_SIZE];
} Person;

Person contacts[MAX_CONTACTS];
int contactCount = 0;

/* Read a line of text safely */
void readLine(const char *prompt, char *buffer, size_t size) {
    int ch;

    printf("%s", prompt);

    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        } else {
            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* Clear the remaining input */
            }
        }
    }
}

/* Read an integer menu choice */
int readChoice(void) {
    char line[50];
    char *end;
    long value;

    if (fgets(line, sizeof(line), stdin) == NULL) {
        return -1;
    }

    value = strtol(line, &end, 10);

    while (*end == ' ' || *end == '\t' ||
           *end == '\n' || *end == '\r') {
        end++;
    }

    if (end == line || *end != '\0' ||
        value < 0 || value > 1000000) {
        return -1;
    }

    return (int)value;
}

/* Display one contact */
void showContact(int index) {
    Person *p = &contacts[index];

    printf("\n--- Contact #%d ---\n", index + 1);
    printf("Last name: %s\n", p->lastName);
    printf("First name: %s\n", p->firstName);
    printf("Middle name: %s\n",
           p->middleName[0] ? p->middleName : "-");
    printf("Workplace: %s\n",
           p->workplace[0] ? p->workplace : "-");
    printf("Position: %s\n",
           p->position[0] ? p->position : "-");
    printf("Phone: %s\n",
           p->phone[0] ? p->phone : "-");
    printf("Email: %s\n",
           p->email[0] ? p->email : "-");
    printf("Social network: %s\n",
           p->socialNetwork[0] ? p->socialNetwork : "-");
    printf("Messenger: %s\n",
           p->messenger[0] ? p->messenger : "-");
}

/* Add a new contact */
void addContact(void) {
    Person p = {0};

    if (contactCount >= MAX_CONTACTS) {
        printf("The contact list is full!\n");
        return;
    }

    printf("\n=== Add contact ===\n");

    do {
        readLine("Last name (required): ",
                 p.lastName, sizeof(p.lastName));

        if (p.lastName[0] == '\0') {
            printf("Last name cannot be empty.\n");
        }
    } while (p.lastName[0] == '\0');

    do {
        readLine("First name (required): ",
                 p.firstName, sizeof(p.firstName));

        if (p.firstName[0] == '\0') {
            printf("First name cannot be empty.\n");
        }
    } while (p.firstName[0] == '\0');

    readLine("Middle name (optional): ",
             p.middleName, sizeof(p.middleName));
    readLine("Workplace (optional): ",
             p.workplace, sizeof(p.workplace));
    readLine("Position (optional): ",
             p.position, sizeof(p.position));
    readLine("Phone (optional): ",
             p.phone, sizeof(p.phone));
    readLine("Email (optional): ",
             p.email, sizeof(p.email));
    readLine("Social network link (optional): ",
             p.socialNetwork, sizeof(p.socialNetwork));
    readLine("Messenger/profile (optional): ",
             p.messenger, sizeof(p.messenger));

    contacts[contactCount] = p;
    contactCount++;

    printf("Contact added successfully!\n");
}

/* Display all contacts */
void showAllContacts(void) {
    int i;

    if (contactCount == 0) {
        printf("The contact list is empty.\n");
        return;
    }

    for (i = 0; i < contactCount; i++) {
        showContact(i);
    }
}

/* Find a contact by its list number */
int selectContact(void) {
    int number;

    if (contactCount == 0) {
        printf("The contact list is empty.\n");
        return -1;
    }

    showAllContacts();
    printf("\nEnter contact number (1-%d): ",
           contactCount);

    number = readChoice();

    if (number < 1 || number > contactCount) {
        printf("Invalid contact number.\n");
        return -1;
    }

    return number - 1;
}

/* Edit an existing contact */
void editContact(void) {
    int index = selectContact();
    Person *p;

    if (index == -1) {
        return;
    }

    p = &contacts[index];

    printf("\n=== Edit contact ===\n");
    printf("Enter a new value for each field.\n");
    printf("Press Enter to keep the current value.\n");

    printf("Current last name: %s\n", p->lastName);
    char value[FIELD_SIZE];

    readLine("New last name: ", value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->lastName, value);
    }

    printf("Current first name: %s\n", p->firstName);
    readLine("New first name: ", value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->firstName, value);
    }

    readLine("New middle name: ", value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->middleName, value);
    }

    readLine("New workplace: ", value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->workplace, value);
    }

    readLine("New position: ", value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->position, value);
    }

    readLine("New phone: ", value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->phone, value);
    }

    readLine("New email: ", value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->email, value);
    }

    readLine("New social network link: ",
             value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->socialNetwork, value);
    }

    readLine("New messenger/profile: ",
             value, sizeof(value));
    if (value[0] != '\0') {
        strcpy(p->messenger, value);
    }

    printf("Contact updated successfully!\n");
}

/* Delete a contact */
void deleteContact(void) {
    int index = selectContact();
    int i;

    if (index == -1) {
        return;
    }

    for (i = index; i < contactCount - 1; i++) {
        contacts[i] = contacts[i + 1];
    }

    contactCount--;

    printf("Contact deleted successfully!\n");
}

/* Main menu */
int main(void) {
    int choice;

    do {
        printf("\n===== PHONE BOOK =====\n");
        printf("1. Add contact\n");
        printf("2. Show all contacts\n");
        printf("3. Edit contact\n");
        printf("4. Delete contact\n");
        printf("0. Exit\n");
        printf("Choose an option: ");

        choice = readChoice();

        switch (choice) {
            case 1:
                addContact();
                break;
            case 2:
                showAllContacts();
                break;
            case 3:
                editContact();
                break;
            case 4:
                deleteContact();
                break;
            case 0:
                printf("Goodbye!\n");
                break;
            default:
                printf("Invalid choice. Try again.\n");
        }
    } while (choice != 0);

    return 0;
}

