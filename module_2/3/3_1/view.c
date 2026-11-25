#include "view.h"
#include "model.h"

#include <stdio.h>
#include <string.h>

int view_read_line(const char *message,
                   char *buffer,
                   size_t size)
{
    printf("%s", message);

    if (fgets(buffer, size, stdin) == NULL)
        return -1;

    buffer[strcspn(buffer, "\n")] = '\0';

    return 0;
}

int view_menu(void)
{
    char input[20];

    printf("\n===== FILE PERMISSIONS =====\n");
    printf("1. Enter permissions\n");
    printf("2. Read permissions from file\n");
    printf("3. Modify current permissions\n");
    printf("0. Exit\n");

    if (view_read_line("Choose: ", input, sizeof(input)) == -1)
        return -1;

    if (strlen(input) != 1)
        return -1;

    if (input[0] >= '0' && input[0] <= '3')
        return input[0] - '0';

    return -1;
}

void view_show_permissions(mode_t permissions)
{
    char symbolic[10];
    char numeric[4];
    char binary[10];

    permissions_to_symbolic(permissions, symbolic);
    permissions_to_numeric(permissions, numeric);
    permissions_to_binary(permissions, binary);

    printf("\n===== PERMISSIONS =====\n");
    printf("Symbolic: %s\n", symbolic);
    printf("Numeric:  %s\n", numeric);
    printf("Binary:   %s\n", binary);
}

void view_show_error(const char *message)
{
    printf("Error: %s\n", message);
}