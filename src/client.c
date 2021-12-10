#include <X11/Xlib.h>
#include <stdlib.h>

#include "devoid.h"
#include "client.h"
#include "dwindle.h"
#include "events.h"
#include "ewmh.h"
#include "../config.h"

void attach(Client *c) {
    if (!head) {
        head = c;
        return;
    }

    Client *i = head;
    while (i -> next != NULL) i = i -> next;
    i -> next = c;
}

void detach(Client *c) {
    if (c == head) {
        head = c -> next;
        return;
    }

    Client *i = head;
    while (i -> next != c) i = i -> next;
    i -> next = c -> next;
}

Client* wintoclient(Window win) {
    for (Client *i = head; i; i = i -> next)
        if (i -> win == win) return i;
    return NULL;
}

void togglefullscreen(Arg arg) {
    (void)arg;
    if (sel == NULL || sel -> isfullscreen != fullscreenlock) return;

    if (!sel -> isfullscreen) {
        sel -> x = 0;
        sel -> y = 0;
        sel -> width = XDisplayWidth(dpy, screen);
        sel -> height = XDisplayHeight(dpy, screen);
        XMoveResizeWindow(dpy, sel -> win, sel -> x, sel -> y, sel -> width, sel -> height);
        XRaiseWindow(dpy, sel -> win);
        CHANGEATOMPROP(net_atoms[NetWMState], XA_ATOM,
                       (unsigned char*)&net_atoms[NetWMStateFullscreen], 1);
    } else {
        tile();
    }

    fullscreenlock = !fullscreenlock;
    sel -> isfullscreen = fullscreenlock;
}

Client* nexttiled(Client *next, unsigned int tags) {
    if (!head || !next) return NULL;

    while (next -> isfloating || !isvisible(next, tags)) {
        next = next -> next;
        if (!next) return NULL;
    }
    return next;
}

Client* prevtiled(Client *c, unsigned int tags) {
    if (!head || !c) return NULL;

    Client *i = head, *prev = NULL;
    do {
        if (!i -> isfloating && isvisible(i, tags)) prev = i;
        i = i -> next;
    } while (i && i != c);

    return prev;
}

Client* nextvisible(Client *next, unsigned int tags) {
    if (!head || !next) return NULL;

    while (!isvisible(next, tags)) {
        next = next -> next;
        if (!next) return NULL;
    }
    return next;
}

Client* prevvisible(Client *c, unsigned int tags) {
    if (!head || !c) return NULL;

    Client *i = head, *prev = NULL;
    do {
        if (isvisible(i, tags)) prev = i;
        i = i -> next;
    } while (i && i != c);

    return prev;
}

Client* get_visible_head() {
    if (!head) return NULL;

    Client *i = head;
    while (i && !isvisible(i, 0)) i = i -> next;

    return i;
}

Client* get_visible_tail() {
    if (!head) return NULL;

    Client *i = head, *prev;
    while (i) {
        if (isvisible(i, 0)) prev = i;
        i = i -> next;
    }

    return prev;
}

Client *newclient(Window win) {
    Client *c;
    if (!(c = (Client *)malloc(sizeof(Client))))
        die("memory allocation failed");

    c -> win = win;
    c -> tags = seltags;
    c -> next = NULL;
    return c;
}

void showhide(Client *c) {
    if (!c) return;

    if (isvisible(c, 0) && c -> isfloating) {
        XMoveWindow(dpy, c -> win, c -> x, c -> y);
        showhide(c -> next);
    } else if (!isvisible(c, 0)) {
        showhide(c -> next);
        XGetWindowAttributes(dpy, c -> win, &attr);
        if (attr.x == XDisplayWidth(dpy, screen)) return;
        c -> x = attr.x;
        c -> y = attr.y;
        c -> width = attr.width;
        c -> height = attr.height;
        XMoveWindow(dpy, c -> win, XDisplayWidth(dpy, screen), XDisplayHeight(dpy, screen));
    } else showhide(c -> next);
}

void killclient(Arg arg) {
    (void)arg;
    if (!sel) return;

    /* send kill signal to window */
    if (!sendevent(sel -> win, XInternAtom(dpy, "WM_DELETE_WINDOW", True))) {
        /* If the client rejects it, we close it down the brutal way */
        XGrabServer(dpy);
        XSetCloseDownMode(dpy, DestroyAll);
        XKillClient(dpy, sel -> win);
        XSync(dpy, False);
        XUngrabServer(dpy);
    }
}

void swap(Client *focused_client, Client *target_client) {
    if (focused_client == NULL || target_client == NULL) return;

    Window temp = focused_client -> win;
    focused_client -> win = target_client -> win;
    target_client -> win = temp;

    resize(focused_client);
    resize(target_client);
}

void zoom(Arg arg) {
    if (!sel) return;

    Client *visible_head = get_visible_head();
    if (!visible_head) return;

    swap(sel, visible_head);
    focus(visible_head);
}

void resize(Client *c) {
    c -> x += gap;
    c -> y += gap;
    c -> width -= gap * 2;
    c -> height -= gap * 2;
    XMoveResizeWindow(dpy, c -> win, c -> x, c -> y, c -> width, c -> height);
}

void incmaster(Arg arg) {
    unsigned int n = MAX(arg.i + nmaster, 1);
    if (n == nmaster) return;
    nmaster = n;
    tile();
}

bool isvisible(Client *c, unsigned int tags) {
    if (!tags) tags = seltags;
    if (c -> tags & tags) return 1;
    return 0;
}

void setmratio(Arg arg) {
    float new_mratio = MIN(0.95, MAX(0.05, mratio + arg.f));
    if (new_mratio == mratio) return;
    mratio = new_mratio;
    tile();
}
