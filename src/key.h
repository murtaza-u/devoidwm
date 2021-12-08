#ifndef KEY_H
#define KEY_H

#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#define MODCLEAN(mask) (mask & \
    (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

typedef union {
    const int i;
} Arg;

typedef struct {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(const Arg arg);
    Arg arg;
} Key;

void handle_keypress(XEvent *event);
void quit(Arg arg);

#endif
