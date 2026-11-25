#include "calculator.h"

#include <string.h>

ParserError calculator_calculate(
    Calculator *calculator,
    const char *expression,
    double *result
) {
    if (calculator == NULL || expression == NULL || result == NULL) {
        return PARSER_INVALID_ARGUMENT;
    }

    calculator->parser.expression = expression;
    calculator->parser.length = strlen(expression);
    calculator->parser.position = 0;
    calculator->parser.error = PARSER_OK;

    *result = parse_calculation(&calculator->parser);

    return calculator->parser.error;
}