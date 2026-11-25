#ifndef VIEW_H
#define VIEW_H

#include <sys/types.h>
#include <stddef.h>

int view_menu(void);

int view_read_line(const char *message,
                   char *buffer,
                   size_t size);

void view_show_permissions(mode_t permissions);

void view_show_error(const char *message);

#endif