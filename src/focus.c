#include "client.h"
#include "devoid.h"
#include "ewmh.h"
#include "events.h"
#include "focus.h"
#include "tags.h"
#include "../config.h"

void focus(Client *c) {
    if (!c || !isvisible(c, 0)) {
        for (c = stack; c && !isvisible(c, 0); c = c -> snext);
        if (!c) {
            XSetInputFocus(dpy, root.win, RevertToPointerRoot, CurrentTime);
            XDeleteProperty(dpy, root.win, net_atoms[NetActiveWindow]);
            sel = NULL;
            return;
        }
    }

    for (Client *i = nextvisible(head, 0); i; i = nextvisible(i -> next, 0)) {
        if (i -> isfullscr) continue;
        XSetWindowBorderWidth(dpy, i -> win, border_width);
        if (i == c) XSetWindowBorder(dpy, i -> win, selbpx);
        else XSetWindowBorder(dpy, i -> win, normbpx);
    }

    XSetInputFocus(dpy, c -> win, RevertToPointerRoot, CurrentTime);

    if (c -> isfloating) XRaiseWindow(dpy, c -> win);

    CHANGEATOMPROP(net_atoms[NetActiveWindow], XA_WINDOW,
                   (unsigned char *)&c -> win, 1);
    sendevent(c -> win, XInternAtom(dpy, "WM_TAKE_FOCUS", False));

    detachstack(c);
    attachstack(c);
    sel = c;
}

void focus_adjacent(Arg arg) {
    if (!sel || getfullscrlock(0)) return;

    if (arg.i > 0) {
        Client *next = nextvisible(sel -> next, 0);
        if (next) focus(next);
        else focus(get_visible_head());
    } else {
        Client *prev = prevvisible(sel, 0);
        if (prev) focus(prev);
        else focus(get_visible_tail());
    }
}

void attachstack(Client *c) {
    c -> snext = stack;
    stack = c;
}

void detachstack(Client *c) {
    if (!stack) return;

    if (stack == c) {
        stack = stack -> snext;
        return;
    }

    Client *i = stack;
    while (i -> snext != c) {
        i = i -> snext;
        if (!i) return;
    }
    i -> snext = c -> snext;
}
