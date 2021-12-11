#ifndef FOCUS_H
#define FOCUS_H

#include "client.h"

void focus(Client *c);
void focus_adjacent(Arg arg);
void attachstack(Client *c);
void detachstack(Client *c);

#endif
