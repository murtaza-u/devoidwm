#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdlib.h>

#include "client.h"
#include "devoid.h"
#include "dwindle.h"
#include "focus.h"
#include "key.h"
#include "ewmh.h"
#include "mouse.h"
#include "tags.h"

void keypress(XEvent *event) {
    handle_keypress(event);
}

void buttonpress(XEvent *event) {
    handle_buttonpress(event);
}

void buttonrelease() {
    XUngrabPointer(dpy, CurrentTime);
}

void motionnotify(XEvent *event) {
    handle_motionnotify(event);
}

void maprequest(XEvent *event) {
    if (get_fullscrlock(seltags)) unlock_fullscr(sel);

    XMapRequestEvent *ev = &event -> xmaprequest;

    XGetWindowAttributes(dpy, ev -> window, &attr);
    if (attr.override_redirect) return; /* docs says window managers must ignore such windows */

    /* emit DestroyNotify and EnterNotify event */
    XSelectInput(dpy, ev -> window, StructureNotifyMask|EnterWindowMask|PropertyChangeMask);

    /* for pinentry-gtk (and maybe some other programs) */
    Client *c;
    if ((c = wintoclient(ev -> window))) {
        XMapWindow(dpy, ev -> window);
        focus(c);
        return;
    }

    c = newclient(ev -> window);

    attach(c);
    apply_window_state(c);
    if (c -> isfloating) {
        XGetWindowAttributes(dpy, c -> win, &attr);
        c -> x = attr.x;
        c -> y = attr.y;
        c -> width = attr.width;
        c -> height = attr.height;
        resize(c);
    } else tile();
    XMapWindow(dpy, ev -> window);
    focus(c);
}

void destroynotify(XEvent *event) {
    XDestroyWindowEvent *ev = &event -> xdestroywindow;
    Client *c;
    if (!(c = wintoclient(ev -> window))) return;
    if (get_fullscrlock(c -> tags)) unlock_fullscr(c);
    unfocus(c);
    detach(c);
    if (isvisible(c, 0)) {
        tile();
        Arg arg = {.i = -1};
        focus_adjacent(arg);
    }
    free(c);
}

void enternotify(XEvent *event) {
    if (sel == NULL) return;
    XCrossingEvent *ev = &event -> xcrossing;
    Client *c;
    if ((c = wintoclient(ev -> window))) focus(c);
}

void clientmessage(XEvent *event) {
    XClientMessageEvent *client_msg_event = &event -> xclient;
    Client *c = wintoclient(client_msg_event -> window);

    if (!c) return;
    if (client_msg_event -> message_type == net_atoms[NetWMState]) {
        if ((unsigned int long)client_msg_event -> data.l[1] == net_atoms[NetWMStatefullscr] ||
                (unsigned int long)client_msg_event -> data.l[2] == net_atoms[NetWMStatefullscr])
            togglefullscr((Arg){0});
    }
}

void handle_event(XEvent *event) {
    switch (event -> type) {
        case KeyPress:
            keypress(event);
            break;

        case ButtonPress:
            buttonpress(event);
            break;

        case ButtonRelease:
            buttonrelease();
            break;

        case MotionNotify:
            motionnotify(event);
            break;

        case MapRequest:
            maprequest(event);
            break;

        case DestroyNotify:
            destroynotify(event);
            break;

        case EnterNotify:
            enternotify(event);
            break;

        case ClientMessage:
            clientmessage(event);
            break;
    }
}

/* Taken from dwm */
bool sendevent(Window win, Atom proto) {
    int n;
    Atom *protocols;
    bool exists = false;
    XEvent ev;

    if (XGetWMProtocols(dpy, win, &protocols, &n)) {
        while (!exists && n--) exists = protocols[n] == proto;
        XFree(protocols);
    }

    if (exists) {
        ev.type = ClientMessage;
        ev.xclient.window = win;
        ev.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = proto;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, win, False, NoEventMask, &ev);
    }
    return exists;
}
