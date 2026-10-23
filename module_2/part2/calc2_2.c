
#include <stdio.h>

/* Function declarations */
double add(double a, double b);
double subtract(double a, double b);
double multiply(double a, double b);
double divide(double a, double b);

/* Addition */
double add(double a, double b) {
    return a + b;
}

/* Subtraction */
double subtract(double a, double b) {
    return a - b;
}

/* Multiplication */
double multiply(double a, double b) {
    return a * b;
}

/* Division */
double divide(double a, double b) {
    return a / b;
}

int main(void) {
    int choice;
    double a, b, result;

    do {
        printf("\n===== CALCULATOR =====\n");
        printf("1. Addition (+)\n");
        printf("2. Subtraction (-)\n");
        printf("3. Multiplication (*)\n");
        printf("4. Division (/)\n");
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

        if (choice < 1 || choice > 4) {
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

        switch (choice) {
            case 1:
                result = add(a, b);
                break;

            case 2:
                result = subtract(a, b);
                break;

            case 3:
                result = multiply(a, b);
                break;

            case 4:
                if (b == 0.0) {
                    printf("Error: division by zero!\n");
                    continue;
                }
                result = divide(a, b);
                break;

            default:
                continue;
        }

        printf("Result: %.2f\n", result);

    } while (1);

    return 0;
}

