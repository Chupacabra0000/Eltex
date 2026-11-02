#include "parser.h"
#include "operators.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

/* Private function declarations */

static void skip_whitespace(Parser *parser);
static double parse_number(Parser *parser);
static double parse_primary(Parser *parser);
static double parse_unary(Parser *parser);
static double parse_term(Parser *parser);
static double parse_expression(Parser *parser);


/* Skip spaces, tabs and other whitespace characters */

static void skip_whitespace(Parser *parser) {
    while (parser->position < parser->length &&
           isspace((unsigned char)parser->expression[parser->position])) {
        parser->position++;
    }
}


/* Parse a numeric value */

static double parse_number(Parser *parser) {
    skip_whitespace(parser);

    const char *start = parser->expression + parser->position;
    char *end;

    errno = 0;

    double value = strtod(start, &end);

    if (end == start) {
        parser->error = PARSER_INVALID_NUMBER;
        return 0.0;
    }

    if (errno == ERANGE || !isfinite(value)) {
        parser->error = PARSER_NUMBER_OUT_OF_RANGE;
        return 0.0;
    }

    parser->position += (size_t)(end - start);

    return value;
}


/* Parse a number or an expression inside parentheses */

static double parse_primary(Parser *parser) {
    skip_whitespace(parser);

    const char *current = parser->expression + parser->position;

    if (*current == '(') {
        parser->position++;

        double result = parse_expression(parser);

        if (parser->error != PARSER_OK) {
            return 0.0;
        }

        skip_whitespace(parser);

        if (parser->position < parser->length &&
            parser->expression[parser->position] == ')') {
            parser->position++;
            return result;
        }

        parser->error = PARSER_MISSING_PARENTHESIS;
        return 0.0;
    }

    return parse_number(parser);
}


/* Handle unary plus and minus */

static double parse_unary(Parser *parser) {
    skip_whitespace(parser);

    const char *current = parser->expression + parser->position;

    if (*current == '-') {
        parser->position++;
        return -parse_unary(parser);
    }

    if (*current == '+') {
        parser->position++;
        return parse_unary(parser);
    }

    return parse_primary(parser);
}


/* Handle multiplication and division */

static double parse_term(Parser *parser) {
    double first = parse_unary(parser);

    if (parser->error != PARSER_OK) {
        return 0.0;
    }

    while (1) {
        skip_whitespace(parser);

        char operation = parser->expression[parser->position];

        if (operation != '*' && operation != '/') {
            break;
        }

        parser->position++;

        double second = parse_unary(parser);

        if (parser->error != PARSER_OK) {
            return 0.0;
        }

        if (operation == '/' && second == 0.0) {
            parser->error = PARSER_DIVISION_BY_ZERO;
            return 0.0;
        }

        if (!apply_operator(operation, first, second, &first)) {
            parser->error = PARSER_SYNTAX_ERROR;
            return 0.0;
        }
    }

    return first;
}


/* Handle addition and subtraction */

static double parse_expression(Parser *parser) {
    double first = parse_term(parser);

    if (parser->error != PARSER_OK) {
        return 0.0;
    }

    while (1) {
        skip_whitespace(parser);

        char operation = parser->expression[parser->position];

        if (operation != '+' && operation != '-') {
            break;
        }

        parser->position++;

        double second = parse_term(parser);

        if (parser->error != PARSER_OK) {
            return 0.0;
        }

        if (!apply_operator(operation, first, second, &first)) {
            parser->error = PARSER_SYNTAX_ERROR;
            return 0.0;
        }
    }

    return first;
}


/* Public parser function */

double parse_calculation(Parser *parser) {
    if (parser == NULL || parser->expression == NULL) {
        if (parser != NULL) {
            parser->error = PARSER_INVALID_ARGUMENT;
        }

        return 0.0;
    }

    parser->position = 0;
    parser->error = PARSER_OK;

    double result = parse_expression(parser);

    if (parser->error != PARSER_OK) {
        return 0.0;
    }

    skip_whitespace(parser);

    if (parser->position != parser->length) {
        parser->error = PARSER_SYNTAX_ERROR;
        return 0.0;
    }

    return result;
}