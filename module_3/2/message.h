
#ifndef MESSAGE_H
#define MESSAGE_H

#include <sys/types.h>

#define MSG_KEY 1234

#define MAX_TEXT 1024
#define MAX_TOPIC_LENGTH 64

#define MSG_TYPE_CONTROL 1

#define BROKER_IDLE_TIMEOUT_SEC 300

typedef struct {
    long mtype;
    char text[MAX_TEXT];
} Message;

#endif
