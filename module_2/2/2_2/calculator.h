#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "parser.h"

typedef struct {
    Parser parser;
} Calculator;

ParserError calculator_calculate(
    Calculator *calculator,
    const char *expression,
    double *result
);

#endif