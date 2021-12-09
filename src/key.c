#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include "devoid.h"
#include "key.h"
#include "../config.h"

void handle_keypress(XEvent *event) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(Key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym &&
            CLEANMASK(keys[i].modifier) == CLEANMASK(event -> xkey.state))
            keys[i].execute(keys[i].arg);
    }
}

void quit(Arg arg) {
    (void)arg;
    isrunning = 0;
}
