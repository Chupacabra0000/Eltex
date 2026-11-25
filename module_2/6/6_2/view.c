#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int readLine(const char *message, char *buffer, int size)
{
    printf("%s", message);

    if (fgets(buffer, size, stdin) == NULL) {
        return 0;
    }

    size_t length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        /* Clear remaining characters from the input line. */
        int character;

        while ((character = getchar()) != '\n' && character != EOF) {
        }
    }

    return 1;
}

static int readNumber(const char *message, int *number)
{
    char buffer[MAX_TEXT];
    char *end;

    if (!readLine(message, buffer, MAX_TEXT)) {
        return 0;
    }

    long value = strtol(buffer, &end, 10);

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    if (end == buffer || *end != '\0') {
        printf("Invalid number.\n");
        return -1;
    }

    *number = (int)value;

    return 1;
}

static void printMenu(void)
{
    printf("\n========== PHONEBOOK ==========\n");
    printf("1. Add contact\n");
    printf("2. Show contacts\n");
    printf("3. Edit contact\n");
    printf("4. Delete contact\n");
    printf("0. Exit\n");
    printf("===============================\n");
}

static void printPerson(const Person *person, int number)
{
    printf("\nContact #%d\n", number);
    printf("-------------------------------\n");
    printf("Surname:      %s\n", person->surname);
    printf("First name:   %s\n", person->firstName);
    printf("Workplace:    %s\n", person->workplace);
    printf("Job title:    %s\n", person->jobTitle);
    printf("Phone:        %s\n", person->phone);
    printf("Email:        %s\n", person->email);
    printf("Social media: %s\n", person->socialMedia);
    printf("Messenger:    %s\n", person->messenger);
}

static void addContactView(Manager *manager)
{
    Person person = {0};

    printf("\n========== ADD CONTACT ==========\n");

    if (!readLine("Surname: ", person.surname, MAX_TEXT) ||
        !readLine("First name: ", person.firstName, MAX_TEXT) ||
        !readLine("Workplace: ", person.workplace, MAX_TEXT) ||
        !readLine("Job title: ", person.jobTitle, MAX_TEXT) ||
        !readLine("Phone: ", person.phone, MAX_TEXT) ||
        !readLine("Email: ", person.email, MAX_TEXT) ||
        !readLine("Social media: ", person.socialMedia, MAX_TEXT) ||
        !readLine("Messenger: ", person.messenger, MAX_TEXT)) {
        return;
    }

    if (person.surname[0] == '\0' ||
        person.firstName[0] == '\0') {
        printf("Surname and first name are required.\n");
        return;
    }

    if (addContact(manager, &person)) {
        printf("Contact added successfully.\n");
    } else {
        printf("Could not add contact. Memory allocation failed.\n");
    }
}

static void showContactsView(Manager *manager)
{
    int count = getContactCount(manager);

    if (count == 0) {
        printf("\nPhonebook is empty.\n");
        return;
    }

    printf("\n========== CONTACTS ==========\n");

    for (int i = 0; i < count; i++) {
        const Person *person = getContact(manager, i);

        if (person != NULL) {
            printPerson(person, i + 1);
        }
    }

    printf("\nTotal contacts: %d\n", count);
}

static void editContactView(Manager *manager)
{
    int count = getContactCount(manager);

    if (count == 0) {
        printf("\nPhonebook is empty.\n");
        return;
    }

    showContactsView(manager);

    int number;
    int result = readNumber("\nEnter contact number to edit: ", &number);

    if (result == 0) {
        return;
    }

    if (result == -1) {
        return;
    }

    if (number < 1 || number > count) {
        printf("Invalid contact number.\n");
        return;
    }

    char surname[MAX_TEXT] = "";
    char firstName[MAX_TEXT] = "";
    char workplace[MAX_TEXT] = "";
    char jobTitle[MAX_TEXT] = "";
    char phone[MAX_TEXT] = "";
    char email[MAX_TEXT] = "";
    char socialMedia[MAX_TEXT] = "";
    char messenger[MAX_TEXT] = "";

    printf("\nEnter new values.\n");
    printf("Leave a field empty to keep its current value.\n\n");

    if (!readLine("New surname: ", surname, MAX_TEXT) ||
        !readLine("New first name: ", firstName, MAX_TEXT) ||
        !readLine("New workplace: ", workplace, MAX_TEXT) ||
        !readLine("New job title: ", jobTitle, MAX_TEXT) ||
        !readLine("New phone: ", phone, MAX_TEXT) ||
        !readLine("New email: ", email, MAX_TEXT) ||
        !readLine("New social media: ", socialMedia, MAX_TEXT) ||
        !readLine("New messenger: ", messenger, MAX_TEXT)) {
        return;
    }

    int success = editContact(manager, number - 1,
        FIELD_SURNAME, surname,
        FIELD_FIRST_NAME, firstName,
        FIELD_WORKPLACE, workplace,
        FIELD_JOB_TITLE, jobTitle,
        FIELD_PHONE, phone,
        FIELD_EMAIL, email,
        FIELD_SOCIAL_MEDIA, socialMedia,
        FIELD_MESSENGER, messenger,
        FIELD_END);

    if (success) {
        printf("\nContact updated successfully.\n");
    } else {
        printf("\nCould not update contact.\n");
    }
}

static void deleteContactView(Manager *manager)
{
    int count = getContactCount(manager);

    if (count == 0) {
        printf("\nPhonebook is empty.\n");
        return;
    }

    showContactsView(manager);

    int number;
    int result = readNumber("\nEnter contact number to delete: ", &number);

    if (result == 0) {
        return;
    }

    if (result == -1) {
        return;
    }

    if (number < 1 || number > count) {
        printf("Invalid contact number.\n");
        return;
    }

    if (deleteContact(manager, number - 1)) {
        printf("Contact deleted successfully.\n");
    } else {
        printf("Could not delete contact.\n");
    }
}

void runPhonebook(Manager *manager)
{
    int choice;

    while (1) {
        printMenu();

        int result = readNumber("Choose an option: ", &choice);

        if (result == 0) {
            break;
        }

        if (result == -1) {
            continue;
        }

        switch (choice) {
            case 1:
                addContactView(manager);
                break;

            case 2:
                showContactsView(manager);
                break;

            case 3:
                editContactView(manager);
                break;

            case 4:
                deleteContactView(manager);
                break;

            case 0:
                printf("Exiting phonebook...\n");
                return;

            default:
                printf("Invalid option. Try again.\n");
        }
    }
}