
#include "view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void readLine(char *buffer, size_t size)
{
    size_t length;

    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        int ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Discard excess input. */
        }
    }
}

static int readNumber(const char *prompt, int *number)
{
    char buffer[MAX_TEXT];
    char *end;
    long value;

    printf("%s", prompt);
    readLine(buffer, sizeof(buffer));

    value = strtol(buffer, &end, 10);

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    if (buffer == end || *end != '\0') {
        return 0;
    }

    if (value < -2147483647L - 1 || value > 2147483647L) {
        return 0;
    }

    *number = (int)value;
    return 1;
}

static void readRequired(const char *prompt, char *buffer)
{
    do {
        printf("%s", prompt);
        readLine(buffer, MAX_TEXT);

        if (buffer[0] == '\0') {
            printf("This field is required.\n");
        }
    } while (buffer[0] == '\0');
}

static void readOptional(const char *prompt, char *buffer)
{
    printf("%s", prompt);
    readLine(buffer, MAX_TEXT);
}

static void showContact(const Person *person, int number)
{
    printf("\n--- Contact %d ---\n", number);
    printf("Surname: %s\n", person->surname);
    printf("First name: %s\n", person->firstName);
    printf("Workplace: %s\n", person->workplace);
    printf("Job title: %s\n", person->jobTitle);
    printf("Phone: %s\n", person->phone);
    printf("Email: %s\n", person->email);
    printf("Social media: %s\n", person->socialMedia);
    printf("Messenger: %s\n", person->messenger);
}

static void addContactView(Manager *manager)
{
    Person person = {0};

    printf("\n=== Add contact ===\n");

    readRequired("Surname: ", person.surname);
    readRequired("First name: ", person.firstName);

    readOptional("Workplace (optional): ", person.workplace);
    readOptional("Job title (optional): ", person.jobTitle);
    readOptional("Phone (optional): ", person.phone);
    readOptional("Email (optional): ", person.email);
    readOptional("Social media (optional): ", person.socialMedia);
    readOptional("Messenger (optional): ", person.messenger);

    if (addContact(manager, &person)) {
        printf("Contact added successfully.\n");
    } else {
        printf("Could not add contact. The phonebook may be full.\n");
    }
}

static void listContactsView(const Manager *manager)
{
    int i;

    if (manager->count == 0) {
        printf("\nThe phonebook is empty.\n");
        return;
    }

    for (i = 0; i < manager->count; i++) {
        const Person *person = getContact(manager, i);

        showContact(person, i + 1);
    }
}

static void editContactView(Manager *manager)
{
    int number;
    char surname[MAX_TEXT] = "";
    char firstName[MAX_TEXT] = "";
    char workplace[MAX_TEXT] = "";
    char jobTitle[MAX_TEXT] = "";
    char phone[MAX_TEXT] = "";
    char email[MAX_TEXT] = "";
    char socialMedia[MAX_TEXT] = "";
    char messenger[MAX_TEXT] = "";

    if (manager->count == 0) {
        printf("\nThe phonebook is empty.\n");
        return;
    }

    listContactsView(manager);

    if (!readNumber("\nEnter contact number to edit: ", &number)) {
        printf("Invalid number.\n");
        return;
    }

    if (number < 1 || number > manager->count) {
        printf("Contact not found.\n");
        return;
    }

    printf("\nEnter new values. Leave a field empty to keep it unchanged.\n");

    readOptional("New surname: ", surname);
    readOptional("New first name: ", firstName);
    readOptional("New workplace: ", workplace);
    readOptional("New job title: ", jobTitle);
    readOptional("New phone: ", phone);
    readOptional("New email: ", email);
    readOptional("New social media: ", socialMedia);
    readOptional("New messenger: ", messenger);

    if (editContact(manager, number - 1,
                    FIELD_SURNAME, surname,
                    FIELD_FIRST_NAME, firstName,
                    FIELD_WORKPLACE, workplace,
                    FIELD_JOB_TITLE, jobTitle,
                    FIELD_PHONE, phone,
                    FIELD_EMAIL, email,
                    FIELD_SOCIAL_MEDIA, socialMedia,
                    FIELD_MESSENGER, messenger,
                    0)) {
        printf("Contact updated successfully.\n");
    } else {
        printf("No fields were changed.\n");
    }
}

static void deleteContactView(Manager *manager)
{
    int number;

    if (manager->count == 0) {
        printf("\nThe phonebook is empty.\n");
        return;
    }

    listContactsView(manager);

    if (!readNumber("\nEnter contact number to delete: ", &number)) {
        printf("Invalid number.\n");
        return;
    }

    if (deleteContact(manager, number - 1)) {
        printf("Contact deleted successfully.\n");
    } else {
        printf("Contact not found.\n");
    }
}

void runView(Manager *manager)
{
    int choice;

    do {
        printf("\n====== PHONEBOOK ======\n");
        printf("1. Add contact\n");
        printf("2. Show all contacts\n");
        printf("3. Edit contact\n");
        printf("4. Delete contact\n");
        printf("0. Exit\n");

        if (!readNumber("Choose an option: ", &choice)) {
            printf("Please enter a valid number.\n");
            continue;
        }

        switch (choice) {
            case 1:
                addContactView(manager);
                break;

            case 2:
                listContactsView(manager);
                break;

            case 3:
                editContactView(manager);
                break;

            case 4:
                deleteContactView(manager);
                break;

            case 0:
                printf("Goodbye!\n");
                break;

            default:
                printf("Unknown option. Try again.\n");
        }
    } while (choice != 0);
}
