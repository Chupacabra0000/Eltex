#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

typedef enum {
    PARSER_OK,
    PARSER_SYNTAX_ERROR,
    PARSER_INVALID_NUMBER,
    PARSER_MISSING_PARENTHESIS,
    PARSER_DIVISION_BY_ZERO,
    PARSER_INVALID_ARGUMENT,
    PARSER_NUMBER_OUT_OF_RANGE
} ParserError;

typedef struct {
    const char *expression;
    size_t length;
    size_t position;
    ParserError error;
} Parser;

double parse_calculation(Parser *parser);

#endif