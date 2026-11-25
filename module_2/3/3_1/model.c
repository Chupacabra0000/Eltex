#include "model.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

int parse_symbolic(const char *input, mode_t *permissions)
{
    const mode_t masks[] = {
        0400, 0200, 0100,
        0040, 0020, 0010,
        0004, 0002, 0001
    };

    const char expected[] = "rwxrwxrwx";

    if (strlen(input) != 9)
        return -1;

    *permissions = 0;

    for (int i = 0; i < 9; i++) {
        if (input[i] == expected[i]) {
            *permissions |= masks[i];   // |= activate individual bits without affecting the previously activated ones. 
        }                               // & checks permission, &= ~ removes permission
        else if (input[i] != '-') {
            return -1;
        }
    }

    return 0;
}

int parse_numeric(const char *input, mode_t *permissions)
{
    char *end;
    long value;

    if (strlen(input) != 3)
        return -1;

    for (int i = 0; i < 3; i++) {
        if (input[i] < '0' || input[i] > '7')
            return -1;
    }

    value = strtol(input, &end, 8);  //8 sets octal literal it can be 2 if binary 16 if hex.....

    if (*end != '\0')
        return -1;

    *permissions = (mode_t)value;

    return 0;
}

void permissions_to_symbolic(mode_t permissions, char *result)
{
    const mode_t masks[] = {
        0400, 0200, 0100,
        0040, 0020, 0010,
        0004, 0002, 0001
    };

    const char letters[] = "rwxrwxrwx";
    //rwxr-xr--
    //0400 0200 0100 0040 0020 0010 0004 0002 0001

    for (int i = 0; i < 9; i++) {
        if (permissions & masks[i])
            result[i] = letters[i];
        else
            result[i] = '-';
    }

    result[9] = '\0';
}

void permissions_to_numeric(mode_t permissions, char *result)
{
    //ASCII conversion '0'+....
    result[0] = '0' + ((permissions >> 6) & 7); //user=6,7,8, right shift by 6 and performing bitwise AND 7=00000111,
    result[1] = '0' + ((permissions >> 3) & 7); 
    result[2] = '0' + (permissions & 7); 

    result[3] = '\0';
}

void permissions_to_binary(mode_t permissions, char *result)
{
    for (int i = 8; i >= 0; i--) {
        result[8 - i] = (permissions & (1 << i)) ? '1' : '0';
    }

    result[9] = '\0';
}


int get_file_permissions(const char *filename, mode_t *permissions)
{
    struct stat file_info;

    if (stat(filename, &file_info) == -1)
        return -1;

    *permissions = file_info.st_mode & 0777;

    return 0;
}


int modify_permissions(mode_t *permissions, const char *command)
{
    mode_t class_mask = 0;
    mode_t new_bits = 0;

    int i = 0;

    /* Determine who is affected */

    while (command[i] != '+' &&
           command[i] != '-' &&
           command[i] != '=' &&
           command[i] != '\0') {

        switch (command[i]) {
            case 'u':
                class_mask |= 0700;
                break;

            case 'g':
                class_mask |= 0070;
                break;

            case 'o':
                class_mask |= 0007;
                break;

            case 'a':
                class_mask |= 0777;
                break;

            default:
                return -1;
        }

        i++;
    }

    if (class_mask == 0 || command[i] == '\0')
        return -1;

    char operation = command[i];
    i++;

    /* Determine which permissions should be changed */

    for (; command[i] != '\0'; i++) {

        mode_t bit = 0;

        switch (command[i]) {
            case 'r':
                bit = 0444;
                break;

            case 'w':
                bit = 0222;
                break;

            case 'x':
                bit = 0111;
                break;

            default:
                return -1;
        }

        new_bits |= bit & class_mask;
    }

    if (operation == '+') {
        *permissions |= new_bits;
    }
    else if (operation == '-') {
        *permissions &= ~new_bits;
    }
    else if (operation == '=') {
        *permissions &= ~class_mask;
        *permissions |= new_bits;
    }
    else {
        return -1;
    }

    return 0;
}