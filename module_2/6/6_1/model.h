#ifndef MODEL_H
#define MODEL_H

#include "list.h"
#include <stdarg.h>

typedef struct {
    List contacts;
} Manager;

typedef enum {
    FIELD_END = 0,
    FIELD_SURNAME,
    FIELD_FIRST_NAME,
    FIELD_WORKPLACE,
    FIELD_JOB_TITLE,
    FIELD_PHONE,
    FIELD_EMAIL,
    FIELD_SOCIAL_MEDIA,
    FIELD_MESSENGER
} Field;

void initManager(Manager *manager);

int addContact(Manager *manager, const Person *person);

int editContact(Manager *manager, int index, ...);

int deleteContact(Manager *manager, int index);

const Person *getContact(const Manager *manager, int index);

int getContactCount(const Manager *manager);

void destroyManager(Manager *manager);

#endif