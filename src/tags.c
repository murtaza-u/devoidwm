#include <X11/Xlib.h>
#include <stdbool.h>

#include "client.h"
#include "dwindle.h"
#include "devoid.h"
#include "tags.h"
#include "focus.h"

void view(Arg arg) {
    if (arg.ui == seltags) return;
    seltags = arg.ui;
    showhide(head);
    focus(NULL);
    if (getfullscrlock(seltags)) lock_fullscr(sel);
    else tile();
    XSync(dpy, True);
}

void toggletag(Arg arg) {
    if (arg.ui == seltags) return;
    seltags ^= arg.ui;
    showhide(head);
    tile();
    XSync(dpy, True);
}

bool getfullscrlock(unsigned int tags) {
    Client *c;
    for (c = stack; c && !isvisible(c, tags); c = c -> snext);
    if (!c) return 0;
    return c -> isfullscr;
}

void tag(Arg arg) {
    if (arg.ui == sel -> tags) return;
    sel -> tags = arg.ui;
    XMoveWindow(dpy, sel -> win, DW, DH);
    if (!sel -> isfloating) tile();
    focus(NULL);
    XSync(dpy, True);
}
