#include "client.h"
#include "devoid.h"
#include "ewmh.h"
#include "events.h"
#include "focus.h"
#include "tags.h"
#include "../config.h"

void focus(Client *c) {
    if (!c) {
        c = get_focus();
        if (!c) {
            c = get_visible_head();
            if (!c ) {
                XSetInputFocus(dpy, root.win, RevertToPointerRoot, CurrentTime);
                XDeleteProperty(dpy, root.win, net_atoms[NetActiveWindow]);
                sel = NULL;
                return;
            }
        }
    }

    XSetInputFocus(dpy, c -> win, RevertToPointerRoot, CurrentTime);
    if (c -> isfloating) XRaiseWindow(dpy, c -> win);

    CHANGEATOMPROP(net_atoms[NetActiveWindow], XA_WINDOW, (unsigned char *)&c -> win, 1);
    sendevent(c -> win, XInternAtom(dpy, "WM_TAKE_FOCUS", False));

    for (Client *i = nexttiled(head, 0); i; i = nexttiled(i -> next, 0)) {
        XSetWindowBorderWidth(dpy, i -> win, border_width);
        if (i == c) XSetWindowBorder(dpy, i -> win, selbpx);
        else XSetWindowBorder(dpy, i -> win, normbpx);
    }

    save_focus(c);
    sel = c;
    XSync(dpy, True);
}

void focus_adjacent(Arg arg) {
    if (!sel) return;

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

void save_focus(Client *c) {
    for (unsigned int i = 0; i < 9; i ++) {
        if ((1 << i) & c -> tags) {
            tags[i].focused = c;
            tags[i].fullscrlock = c -> isfullscr;
        }
    }
}

Client* get_focus() {
    for (unsigned int i = 0; i < 9; i ++)
        if ((1 << i) & seltags) return tags[i].focused;
    return NULL;
}

Client* unfocus(Client *c) {
    Client *tofocus = NULL;
    for (unsigned int i = 0; i < 9; i ++) {
        if (((1 << i) & c -> tags) && tags[i].focused == c)
            tags[i].focused = tofocus = prevvisible(c, c -> tags);
    }
    return tofocus;
}
