#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>
#include <stdlib.h>

#include "client.h"
#include "devoid.h"
#include "dwindle.h"
#include "focus.h"
#include "key.h"
#include "ewmh.h"
#include "mouse.h"
#include "rules.h"
#include "tags.h"

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

void keypress(XEvent *event) {
    handle_keypress(event);
}

void buttonpress(XEvent *event) {
    handle_buttonpress(event);
}

void buttonrelease(XEvent *event) {
    (void)event;
    XUngrabPointer(dpy, CurrentTime);
}

void motionnotify(XEvent *event) {
    handle_motionnotify(event);
}

void maprequest(XEvent *event) {
    if (getfullscrlock(seltags)) unlock_fullscr(sel);

    XMapRequestEvent *ev = &event -> xmaprequest;

    if (!(XGetWindowAttributes(dpy, ev -> window, &attr))) return;

    /* docs says window managers must ignore such windows */
    if (attr.override_redirect) return;

    /* emit DestroyNotify and EnterNotify event */
    XSelectInput(dpy, ev -> window,
                StructureNotifyMask|EnterWindowMask|PropertyChangeMask);

    /* for pinentry-gtk (and maybe some other programs) */
    Client *c;
    if ((c = wintoclient(ev -> window))) {
        focus(c);
        XMapWindow(dpy, ev -> window);
        return;
    }

    c = newclient(ev -> window);

    attach(c);
    apply_window_state(c);
    apply_rules(c);

    if (c -> isfloating) {
        XGetWindowAttributes(dpy, c -> win, &attr);
        c -> x = attr.x;
        c -> y = attr.y;
        c -> width = attr.width;
        c -> height = attr.height;
    } else if (c -> isfullscr) lock_fullscr(c);
    else tile();

    XMapWindow(dpy, ev -> window);
    focus(c);
    XSync(dpy, True);
}

void destroynotify(XEvent *event) {
    XDestroyWindowEvent *ev = &event -> xdestroywindow;
    Client *c;
    if (!(c = wintoclient(ev -> window))) return;
    detach(c);
    detachstack(c);
    if (isvisible(c, 0)) {
        if (!c -> isfloating || c -> isfullscr) tile();
        focus(NULL);
    }
    if (getfullscrlock(c -> tags)) unlock_fullscr(c);
    XDestroyWindow(dpy, ev -> window);
    free(c);
    XSync(dpy, True);
}

void enternotify(XEvent *event) {
    if (sel == NULL) return;
    XCrossingEvent *ev = &event -> xcrossing;
    Client *c = NULL;
    if ((c = wintoclient(ev -> window))) focus(c);
}

void clientmessage(XEvent *event) {
    XClientMessageEvent *ev = &event -> xclient;

    Client *c;
    if (!(c = wintoclient(ev -> window))) return;

    if (ev -> message_type == net_atoms[NetWMState]) {
        if ((unsigned int long)ev -> data.l[1] == net_atoms[NetWMStateFullscreen] ||
            (unsigned int long)ev -> data.l[2] == net_atoms[NetWMStateFullscreen]) {
            if (c -> isfullscr) unlock_fullscr(c);
            else lock_fullscr(c);
        }
    }
}

void unmapnotify(XEvent *event) {
    XUnmapEvent *ev = &event -> xunmap;
    Client *c;

    if ((c = wintoclient(ev -> window))) {
		if (ev -> send_event) {
            long data[] = {WithdrawnState, None};
            XChangeProperty(dpy, c->win, XInternAtom(dpy, "WM_STATE", False),
                            XInternAtom(dpy, "WM_STATE", False), 32,
                            PropModeReplace, (unsigned char *)data, 2);
        } else {
            if (ev->send_event) {
                long data[] = { WithdrawnState, None };
                XChangeProperty(dpy, c->win, XInternAtom(dpy, "WM_STATE", False),
                                XInternAtom(dpy, "WM_STATE", False), 32,
                                PropModeReplace, (unsigned char *)data, 2);
            } else {
                detach(c);
                detachstack(c);
                free(c);
                tile();
                focus(NULL);
            }
        }
    }
}

void configurerequest(XEvent *event) {
    Client *c;
    XConfigureRequestEvent *ev = &event -> xconfigurerequest;
    XWindowChanges wc;

    if (!(c = wintoclient(ev -> window))) {
        wc.x = ev -> x;
        wc.y = ev -> y;
        wc.width = ev -> width;
        wc.height = ev -> height;
        wc.border_width = ev -> border_width;
        wc.sibling = ev -> above;
        wc.stack_mode = ev -> detail;
        XConfigureWindow(dpy, ev -> window, ev -> value_mask, &wc);
    }
}

void (*handle_events[LASTEvent])(XEvent *event) = {
    [KeyPress] = keypress,
    [ButtonPress] = buttonpress,
    [ButtonRelease] = buttonrelease,
    [MotionNotify] = motionnotify,
    [MapRequest] = maprequest,
    [DestroyNotify] = destroynotify,
    [EnterNotify] = enternotify,
    [ClientMessage] = clientmessage,
    [UnmapNotify] = unmapnotify,
    [ConfigureRequest] = configurerequest,
};
