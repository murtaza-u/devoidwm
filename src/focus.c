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

    CHANGEATOMPROP(net_atoms[NetActiveWindow], XA_WINDOW, (unsigned char *)&c -> win, 1);
    sendevent(c -> win, XInternAtom(dpy, "WM_TAKE_FOCUS", False));
    XSetInputFocus(dpy, c -> win, RevertToPointerRoot, CurrentTime);
    if (c -> isfloating) XRaiseWindow(dpy, c -> win);

    for (Client *i = nexttiled(head, 0); i; i = nexttiled(i -> next, 0)) {
        XSetWindowBorderWidth(dpy, i -> win, border_width);
        if (i == c) XSetWindowBorder(dpy, i -> win, selbpx);
        else XSetWindowBorder(dpy, i -> win, normbpx);
    }

    detachstack(c);
    attachstack(c);
    sel = c;
    XSync(dpy, True);
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
    if (!stack) {
        stack = c;
        return;
    }

    c -> snext = stack;
    stack = c;
}

void detachstack(Client *c) {
    if (!stack) return;

    if (stack == c) {
        stack = c -> snext;
        return;
    }

    Client *i = stack;
    while (i -> snext != c) {
        i = i -> snext;
        if (!i) return;
    }
    i -> snext = c -> snext;
}
