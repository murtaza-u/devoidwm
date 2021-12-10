#ifndef DWINDLE_H
#define DWINDLE_H

#include "client.h"

void tile();
void dwindle(Client *c, Client *prev, unsigned int i, unsigned int n, unsigned int mwidth);
void shrink(Client *c, int *x, int *y, unsigned int *width, unsigned int *height);
void expand(Client *c, int x, int y);

#endif
