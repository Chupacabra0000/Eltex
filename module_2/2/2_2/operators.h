#ifndef OPERATORS_H
#define OPERATORS_H

typedef struct {
    char symbol;
    double (*function)(double, double);
} Operator;

int apply_operator(char symbol, double left, double right, double *result);

#endif