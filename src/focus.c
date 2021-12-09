#include "client.h"
#include "devoid.h"
#include "ewmh.h"
#include "events.h"

void focus(Client *c) {
    if (!c) {
        // XSetInputFocus(dpy, root.win, RevertToPointerRoot, CurrentTime);
        // XDeleteProperty(dpy, root.win, net_atoms[NetActiveWindow]);
        return;
    }

    XSetInputFocus(dpy, c -> win, RevertToPointerRoot, CurrentTime);
    if (c -> isfloating) XRaiseWindow(dpy, c -> win);

    CHANGEATOMPROP(net_atoms[NetActiveWindow], XA_WINDOW, (unsigned char *)&c -> win, 1);
    sendevent(c -> win, XInternAtom(dpy, "WM_TAKE_FOCUS", False));

    sel = c;
}

void focus_adjacent(Arg arg) {
    if (!sel) return;

    if (arg.i > 0) {
        Client *next = nextvisible(sel -> next);
        if (next) focus(next);
        else focus(get_visible_head());
    } else {
        Client *prev = prevvisible(sel);
        if (prev) focus(prev);
        else focus(get_visible_tail());
    }
}
