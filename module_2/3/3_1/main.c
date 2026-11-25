#include "model.h"
#include "view.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    mode_t current_permissions = 0;
    int has_permissions = 0;

    char input[32];
    char filename[256];
    char command[64];

    while (1) {

        int choice = view_menu();

        if (choice == -2)
            break;

        if (choice == -1) {
            view_show_error("Invalid menu choice.");
            continue;
        }

        if (choice == 0)
            break;

        switch (choice) {

            /* OPTION 1: ENTER PERMISSIONS */

            case 1: {
                mode_t temp;

                if (view_read_line(
                    "Enter symbolic or numeric permissions: ",
                    input, sizeof(input)) == -1)
                    return 0;

                int result;

                if (strlen(input) == 9) {
                    result = parse_symbolic(input, &temp);
                }
                else if (strlen(input) == 3) {
                    result = parse_numeric(input, &temp);
                }
                else {
                    result = -1;
                }

                if (result == -1) {
                    view_show_error("Invalid permissions.");
                    break;
                }

                current_permissions = temp;
                has_permissions = 1;

                view_show_permissions(current_permissions);

                break;
            }

            /* OPTION 2: READ FILE PERMISSIONS */

            case 2:

                if (view_read_line(
                    "Enter filename: ",
                    filename, sizeof(filename)) == -1)
                    return 0;

                if (get_file_permissions(
                    filename, &current_permissions) == -1) {

                    view_show_error("Cannot retrieve file information.");
                    break;
                }

                has_permissions = 1;

                view_show_permissions(current_permissions);

                break;

            /* OPTION 3: MODIFY PERMISSIONS */

            case 3:

                if (!has_permissions) {
                    view_show_error(
                        "Enter permissions or select a file first.");
                    break;
                }

                if (view_read_line(
                    "Enter modification command (e.g. u+x): ",
                    command, sizeof(command)) == -1)
                    return 0;

                if (modify_permissions(
                    &current_permissions, command) == -1) {

                    view_show_error("Invalid modification command.");
                    break;
                }

                view_show_permissions(current_permissions);

                break;
        }
    }

    printf("Program finished.\n");

    return 0;
}