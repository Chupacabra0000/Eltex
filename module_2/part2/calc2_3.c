
#include <stdio.h>

/* All operations have the same function signature */
typedef double (*OperationFunction)(double, double);

/* Structure describing one calculator operation */
typedef struct {
    const char *name;
    char symbol;
    OperationFunction function;
} Operation;

/* Mathematical functions */
double add(double a, double b) {
    return a + b;
}

double subtract(double a, double b) {
    return a - b;
}

double multiply(double a, double b) {
    return a * b;
}

double divide(double a, double b) {
    return a / b;
}

/* Add new operations here */
Operation operations[] = {
    {"Addition", '+', add},
    {"Subtraction", '-', subtract},
    {"Multiplication", '*', multiply},
    {"Division", '/', divide}
};

int main(void) {
    int choice;
    double a, b, result;

    int operationCount =
        (int)(sizeof(operations) / sizeof(operations[0]));

    do {
        printf("\n===== CALCULATOR =====\n");

        for (int i = 0; i < operationCount; i++) {
            printf("%d. %s (%c)\n",
                   i + 1,
                   operations[i].name,
                   operations[i].symbol);
        }

        printf("0. Exit\n");
        printf("Choose operation: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            return 1;
        }

        if (choice == 0) {
            printf("Goodbye!\n");
            break;
        }

        if (choice < 1 || choice > operationCount) {
            printf("Unknown operation.\n");
            continue;
        }

        printf("Enter first number: ");
        if (scanf("%lf", &a) != 1) {
            printf("Invalid number.\n");
            return 1;
        }

        printf("Enter second number: ");
        if (scanf("%lf", &b) != 1) {
            printf("Invalid number.\n");
            return 1;
        }

        Operation selected = operations[choice - 1];

        if (selected.symbol == '/' && b == 0.0) {
            printf("Error: division by zero!\n");
            continue;
        }

        /* Call the selected function through its pointer */
        result = selected.function(a, b);

        printf("Result: %.2f\n", result);

    } while (1);

    return 0;
}

