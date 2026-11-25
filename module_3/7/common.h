
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define PORT 50000
#define MAX_CLIENTS 32
#define MAX_NAME 32
#define MAX_FILENAME 256
#define MAX_DATA 32768
#define MAX_FRAME (MAX_DATA + 8)
#define BUFFER_SIZE 65536

#define MSG_NAME       1
#define MSG_CHAT       2
#define MSG_SYSTEM     3
#define MSG_FILE_BEGIN 4
#define MSG_FILE_DATA  5
#define MSG_FILE_END   6

// Every TCP frame starts with this header.
typedef struct {
    uint32_t type;
    uint32_t length;
} FrameHeader;

#endif
