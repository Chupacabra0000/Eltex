
#ifndef OPERATORS_H
#define OPERATORS_H

typedef struct {
    char symbol;
    double (*function)(double, double);
    void *library;
} Operator;

/* Load all supported libraries from a directory */
int operators_load_directory(const char *directory);

/* Apply an operation */
int apply_operator(
    char symbol,
    double left,
    double right,
    double *result
);

/* Unload all dynamic libraries */
void operators_unload_all(void);

#endif
