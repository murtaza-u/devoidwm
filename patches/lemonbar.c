#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <strings.h>
#include <stdlib.h>

#include "lemonbar.h"
#include "devoid.h"
#include "dwindle.h"
#include "../config.h"

bool isBarVisible = 1;

Window getBar() {
    unsigned int n;
    Window root_return, parent_return, *children;

    if (XQueryTree(dpy, root.win, &parent_return, &root_return, &children, &n)) {
        for (unsigned int i = 0; i < n; i ++) {
            XClassHint hints = {NULL, NULL};
            XGetClassHint(dpy, children[i], &hints);

            if (hints.res_class != NULL && strcasecmp(hints.res_class, "Bar") == 0) {
                return children[i];
            }
        }
    }

    return 0;
}

void hide(Window win) {
    XUnmapWindow(dpy, win);
    root.y = 0;
    root.height = XDisplayHeight(dpy, screen);
    tile();
}

void show(Window win) {
    XMapWindow(dpy, win);
    root.y = margin_top;
    root.height = XDisplayHeight(dpy, screen) - (margin_top + margin_bottom);
    tile();
}

void toggleBar(Arg arg) {
    (void)arg;

    Window bar = getBar();
    if (!bar) return;

    if (isBarVisible) hide(bar);
    else show(bar);

    isBarVisible = !isBarVisible;
}
