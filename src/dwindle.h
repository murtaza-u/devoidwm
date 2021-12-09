#ifndef DWINDLE_H
#define DWINDLE_H

#include "client.h"

void tile();
void shrink(Client *c, int *x, int *y, unsigned int *width, unsigned int *height);
void expand(Client *c, int x, int y);

#endif
