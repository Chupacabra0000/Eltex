
#include "operators.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>

#define MAX_OPERATORS 32

static Operator operators[MAX_OPERATORS];
static int operators_count = 0;

/*
 * Determine the arithmetic symbol from the library filename.
 *
 * add.so      -> +
 * subtract.so -> -
 * multiply.so -> *
 * divide.so   -> /
 */
static char get_operator_symbol(const char *filename) {
    if (strcmp(filename, "add.so") == 0) {
        return '+';
    }

    if (strcmp(filename, "subtract.so") == 0) {
        return '-';
    }

    if (strcmp(filename, "multiply.so") == 0) {
        return '*';
    }

    if (strcmp(filename, "divide.so") == 0) {
        return '/';
    }

    return '\0';
}

/* Check whether an operator is already registered */
static int operator_exists(char symbol) {
    for (int i = 0; i < operators_count; i++) {
        if (operators[i].symbol == symbol) {
            return 1;
        }
    }

    return 0;
}

/* Load all dynamic libraries from a directory */
int operators_load_directory(const char *directory) {
    if (directory == NULL) {
        return 0;
    }

    DIR *dir = opendir(directory);

    if (dir == NULL) {
        perror("Cannot open plugins directory");
        return 0;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        /* Ignore files that are not .so libraries */
        const char *extension = strrchr(entry->d_name, '.');

        if (extension == NULL || strcmp(extension, ".so") != 0) {
            continue;
        }

        /* Determine the operation from the filename */
        char symbol = get_operator_symbol(entry->d_name);

        if (symbol == '\0') {
            printf("Unknown library: %s\n", entry->d_name);
            continue;
        }

        if (operator_exists(symbol)) {
            printf("Operation already loaded: %c\n", symbol);
            continue;
        }

        if (operators_count >= MAX_OPERATORS) {
            printf("Maximum number of operators reached.\n");
            break;
        }

        /* Construct the full library path */
        char path[1024];

        int length = snprintf(
            path,
            sizeof(path),
            "%s/%s",
            directory,
            entry->d_name
        );

        if (length < 0 || (size_t)length >= sizeof(path)) {
            printf("Library path is too long: %s\n", entry->d_name);
            continue;
        }

        /* Load the dynamic library */
        void *library = dlopen(path, RTLD_NOW);

        if (library == NULL) {
            fprintf(
                stderr,
                "Cannot load %s: %s\n",
                path,
                dlerror()
            );
            continue;
        }

        /* Clear previous dynamic loader errors */
        dlerror();

        /* Find the function named "operation" */
        void *function_address = dlsym(library, "operation");

        const char *error = dlerror();

        if (error != NULL) {
            fprintf(
                stderr,
                "Cannot find operation in %s: %s\n",
                path,
                error
            );

            dlclose(library);
            continue;
        }

        /* Convert the address into a function pointer */
        double (*function)(double, double);

        *(void **)(&function) = function_address;

        /* Register the operation */
        operators[operators_count].symbol = symbol;
        operators[operators_count].function = function;
        operators[operators_count].library = library;

        operators_count++;

        printf("Loaded operation: %c (%s)\n", symbol, path);
    }

    closedir(dir);

    return operators_count;
}

/* Find and execute an operation */
int apply_operator(
    char symbol,
    double left,
    double right,
    double *result
) {
    if (result == NULL) {
        return 0;
    }

    for (int i = 0; i < operators_count; i++) {

        if (operators[i].symbol == symbol) {

            *result = operators[i].function(left, right);

            return 1;
        }
    }

    return 0;
}

/* Close all loaded dynamic libraries */
void operators_unload_all(void) {
    for (int i = 0; i < operators_count; i++) {

        if (operators[i].library != NULL) {
            dlclose(operators[i].library);
        }
    }

    operators_count = 0;
}
