
#ifndef MODEL_H
#define MODEL_H

#define MAX_CONTACTS 100
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

typedef struct {
    Person contacts[MAX_CONTACTS];
    int count;
} Manager;

enum Field {
    FIELD_SURNAME = 1,
    FIELD_FIRST_NAME,
    FIELD_WORKPLACE ,
    FIELD_JOB_TITLE,
    FIELD_PHONE,
    FIELD_EMAIL,
    FIELD_SOCIAL_MEDIA,
    FIELD_MESSENGER
};

void initManager(Manager *manager);

int addContact(Manager *manager, const Person *person);
int editContact(Manager *manager, int index, ...);
int deleteContact(Manager *manager, int index);

const Person *getContact(const Manager *manager, int index);

#endif
