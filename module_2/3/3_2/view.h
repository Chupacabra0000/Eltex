#ifndef VIEW_H
#define VIEW_H

#include "model.h"

void view_show_network(const Network *network);

void view_show_statistics(const Statistics *statistics);

void view_show_error(const char *message);

#endif
