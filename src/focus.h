#ifndef FOCUS_H
#define FOCUS_H

#include "client.h"

void focus(Client *c);
void focus_adjacent(Arg arg);
void save_focus(Client *c);
Client* get_focus();
Client* unfocus(Client *c);

#endif
