#include "view.h"

#include <stdio.h>
#include <string.h>

#define INPUT_SIZE 256

static void display_result(double result) {
    printf("Result: %.10g\n", result);
}

static void display_error(ParserError error) {
    switch (error) {
        case PARSER_SYNTAX_ERROR:
            printf("Error: Invalid expression syntax.\n");
            break;

        case PARSER_INVALID_NUMBER:
            printf("Error: Invalid number.\n");
            break;

        case PARSER_MISSING_PARENTHESIS:
            printf("Error: Missing closing parenthesis.\n");
            break;

        case PARSER_DIVISION_BY_ZERO:
            printf("Error: Division by zero.\n");
            break;

        case PARSER_NUMBER_OUT_OF_RANGE:
            printf("Error: Number is out of range.\n");
            break;

        case PARSER_INVALID_ARGUMENT:
            printf("Error: Invalid argument.\n");
            break;

        case PARSER_OK:
            break;

        default:
            printf("Error: Unknown error.\n");
            break;
    }
}

void run_calculator(Calculator *calculator) {
    char expression[INPUT_SIZE];
    double result;

    printf("=== Calculator ===\n");
    printf("Enter an expression or type 'exit' to quit.\n");

    while (1) {
        printf("\n> ");

        if (fgets(expression, sizeof(expression), stdin) == NULL) {
            break;
        }

        expression[strcspn(expression, "\n")] = '\0';

        if (strcmp(expression, "exit") == 0) {
            break;
        }

        if (expression[0] == '\0') {
            continue;
        }

        ParserError error = calculator_calculate(
            calculator,
            expression,
            &result
        );

        if (error == PARSER_OK) {
            display_result(result);
        } else {
            display_error(error);
        }
    }

    printf("Calculator closed.\n");
}