#include "operators.h"

static double add(double a, double b) {
    return a + b;
}

static double subtract(double a, double b) {
    return a - b;
}

static double multiply(double a, double b) {
    return a * b;
}

static double divide(double a, double b) {
    return a / b;
}

static const Operator operators[] = {
    {'+', add},
    {'-', subtract},
    {'*', multiply},
    {'/', divide}
};

static const int operators_count =
    sizeof(operators) / sizeof(operators[0]);

int apply_operator(char symbol, double left, double right, double *result) {
    for (int i = 0; i < operators_count; i++) {
        if (operators[i].symbol == symbol) {
            *result = operators[i].function(left, right);
            return 1;
        }
    }

    return 0;
}