#ifndef DWINDLE_H
#define DWINDLE_H

#include "client.h"
#include "key.h"

void tile();
void dwindle(Client *c, Client *prev, unsigned int i, unsigned int n, unsigned int mw);
void shrink(Client *c, int *x, int *y, unsigned int *w, unsigned int *h);
void setlayout(Arg);

#endif
