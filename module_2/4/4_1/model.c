#include "model.h"

#include <string.h>
#include <stdarg.h>

static void copyText(char *destination, const char *source)
{
    if (source == NULL) {
        return;
    }

    strncpy(destination, source, MAX_TEXT - 1);
    destination[MAX_TEXT - 1] = '\0';
}

static void updateField(Person *person, Field field, const char *value)
{
    switch (field) {
        case FIELD_SURNAME:
            copyText(person->surname, value);
            break;

        case FIELD_FIRST_NAME:
            copyText(person->firstName, value);
            break;

        case FIELD_WORKPLACE:
            copyText(person->workplace, value);
            break;

        case FIELD_JOB_TITLE:
            copyText(person->jobTitle, value);
            break;

        case FIELD_PHONE:
            copyText(person->phone, value);
            break;

        case FIELD_EMAIL:
            copyText(person->email, value);
            break;

        case FIELD_SOCIAL_MEDIA:
            copyText(person->socialMedia, value);
            break;

        case FIELD_MESSENGER:
            copyText(person->messenger, value);
            break;

        default:
            break;
    }
}

void initManager(Manager *manager)
{
    initList(&manager->contacts);
}

int addContact(Manager *manager, const Person *person)
{
    if (manager == NULL || person == NULL) {
        return 0;
    }

    if (person->surname[0] == '\0' ||
        person->firstName[0] == '\0') {
        return 0;
    }

    return insertSorted(&manager->contacts, person);
}

int editContact(Manager *manager, int index, ...)
{
    if (manager == NULL) {
        return 0;
    }

    Node *node = getNode(&manager->contacts, index);

    if (node == NULL) {
        return 0;
    }

    int surnameChanged = 0;

    va_list args;
    va_start(args, index);

    while (1) {
        int field = va_arg(args, int);

        if (field == FIELD_END) {
            break;
        }

        char *value = va_arg(args, char *);

        /* An empty string means "leave this field unchanged". */
        if (value == NULL || value[0] == '\0') {
            continue;
        }

        if (field == FIELD_SURNAME) {
            surnameChanged = 1;
        }

        updateField(&node->person, (Field)field, value);
    }

    va_end(args);

    /* The node must be moved if its surname has changed. */
    if (surnameChanged) {
        repositionNode(&manager->contacts, node);
    }

    return 1;
}

int deleteContact(Manager *manager, int index)
{
    if (manager == NULL) {
        return 0;
    }

    Node *node = getNode(&manager->contacts, index);

    if (node == NULL) {
        return 0;
    }

    removeNode(&manager->contacts, node);

    return 1;
}

const Person *getContact(const Manager *manager, int index)
{
    if (manager == NULL) {
        return NULL;
    }

    const Node *node = getNodeConst(&manager->contacts, index);

    if (node == NULL) {
        return NULL;
    }

    return &node->person;
}

int getContactCount(const Manager *manager)
{
    if (manager == NULL) {
        return 0;
    }

    return manager->contacts.count;
}

void destroyManager(Manager *manager)
{
    if (manager == NULL) {
        return;
    }

    destroyList(&manager->contacts);
}