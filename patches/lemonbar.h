#ifndef LEMONBAR_H
#define LEMONBAR_H

#include <stdbool.h>

#include "key.h"

extern bool isBarVisible;

Window getBar();
void toggleBar(Arg arg);

#endif
