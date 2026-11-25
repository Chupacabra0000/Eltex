#ifndef MODEL_H
#define MODEL_H

#include <sys/types.h>

typedef struct {
    mode_t permissions;
} Permission;

/* Conversion */
int parse_symbolic(const char *input, mode_t *permissions);
int parse_numeric(const char *input, mode_t *permissions);

/* Representation */
void permissions_to_symbolic(mode_t permissions, char *result);
void permissions_to_numeric(mode_t permissions, char *result);
void permissions_to_binary(mode_t permissions, char *result);

/* File information */
int get_file_permissions(const char *filename, mode_t *permissions);

/* Modification */
int modify_permissions(mode_t *permissions, const char *command);

#endif