#ifndef RULES_H
#define RULES_H

#include "client.h"

typedef struct Rule {
    char *classname, *instance;
    bool isfloating, isfullscreen;
} Rule;

void apply_rules(Client *client);

#endif
