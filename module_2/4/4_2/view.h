
#ifndef VIEW_H
#define VIEW_H

#include "model.h"

void view_show_menu(void);

void view_show_queue(const PriorityQueue *queue);

void view_show_message(const Message *message);

void view_show_error(const char *message);

int view_read_integer(const char *prompt, int *value);

int view_read_message(Message *message);

#endif

