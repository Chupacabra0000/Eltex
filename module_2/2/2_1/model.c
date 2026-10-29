
#include "model.h"
#include <stdarg.h>
#include <string.h>

static int isEmpty(const char *text)
{
    return text[0] == '\0';
}

static void copyText(char *destination, const char *source)
{
    int i = 0;

    while (i < MAX_TEXT - 1 && source[i] != '\0') {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

void initManager(Manager *manager)
{
    manager->count = 0;
}

int addContact(Manager *manager, const Person *person)
{
    if (manager->count >= MAX_CONTACTS) {
        return 0;
    }

    if (isEmpty(person->surname) ||
        isEmpty(person->firstName)) {
        return 0;
    }

    manager->contacts[manager->count] = *person;
    manager->count++;

    return 1;
}

int editContact(Manager *manager, int index, ...)
{
    va_list args;
    int field;
    int changed = 0;
    Person *person;

    if (index < 0 || index >= manager->count) {
        return 0;
    }

    person = &manager->contacts[index];

    va_start(args, index);

    while ((field = va_arg(args, int)) != 0) {
        char *value = va_arg(args, char *);

        if (value[0] == '\0') {
            continue;
        }

        switch (field) {
            case FIELD_SURNAME:
                copyText(person->surname, value);
                changed = 1;
                break;
                
            case FIELD_FIRST_NAME:
                copyText(person->firstName, value);
                changed = 1;
                break;

            case FIELD_WORKPLACE:
                copyText(person->workplace, value);
                changed = 1;
                break;

            case FIELD_JOB_TITLE:
                copyText(person->jobTitle, value);
                changed = 1;
                break;

            case FIELD_PHONE:
                copyText(person->phone, value);
                changed = 1;
                break;

            case FIELD_EMAIL:
                copyText(person->email, value);
                changed = 1;
                break;

            case FIELD_SOCIAL_MEDIA:
                copyText(person->socialMedia, value);
                changed = 1;
                break;

            case FIELD_MESSENGER:
                copyText(person->messenger, value);
                changed = 1;
                break;

            default:
                break;
        }
    }

    va_end(args);

    return changed;
}

int deleteContact(Manager *manager, int index)
{
    int i;

    if (index < 0 || index >= manager->count) {
        return 0;
    }

    for (i = index; i < manager->count - 1; i++) {
        manager->contacts[i] = manager->contacts[i + 1];
    }

    manager->count--;

    return 1;
}

const Person *getContact(const Manager *manager, int index)
{
    if (index < 0 || index >= manager->count) {
        return NULL;
    }

    return &manager->contacts[index];
}
