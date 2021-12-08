#include <X11/Xlib.h>
#include <stdbool.h>

#include "devoid.h"
#include "key.h"
#include "ewmh.h"

void keypress(XEvent *event) {
    handle_keypress(event);
}

void buttonpress(XEvent *event) {

}

void buttonrelease(XEvent *event) {

}

void motionnotify(XEvent *event) {

}

void maprequest(XEvent *event) {
    XMapRequestEvent *ev = &event -> xmaprequest;

    XGetWindowAttributes(dpy, ev -> window, &attr);
    if (attr.override_redirect) return; /* docs says window managers must ignore such windows */

    /* emit DestroyNotify and EnterNotify event */
    XSelectInput(dpy, ev -> window, StructureNotifyMask|EnterWindowMask|PropertyChangeMask);

    /* for pinentry-gtk (and maybe some other programs) */
    Client *c = root.head;
    do {
        if (c == NULL) break;
        if(ev -> window == c -> win) {
            XMapWindow(dpy, ev -> window);
            focus(c);
            return;
        }
        c = c -> next;
    } while (c != NULL && c -> next != root.head);

    XMapWindow(dpy, ev -> window);
    XMoveResizeWindow(dpy, ev -> window, root.x, root.y, root.width, root.height);
    XSync(dpy, True);
}

void destroynotify(XEvent *event) {

}

void enternotify(XEvent *event) {
    if (root.focused == NULL) return;
    XCrossingEvent *ev = &event -> xcrossing;
    Client *client = root.head;
    do {
        if (client -> win == ev -> window) {
            focus(client);
            break;
        }
        client = client -> next;
    } while (client != NULL && client != root.head);
}

void clientmessage(XEvent *event) {
    XClientMessageEvent *client_msg_event = &event -> xclient;
    Client *c = wintoclient(client_msg_event -> window);

    if (!c) return;
    if (client_msg_event -> message_type == net_atoms[NetWMState]) {
        if ((unsigned int long)client_msg_event -> data.l[1] == net_atoms[NetWMStateFullscreen] ||
                (unsigned int long)client_msg_event -> data.l[2] == net_atoms[NetWMStateFullscreen])
            return;
            // toggle_fullscreen((Arg){0});
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
            buttonrelease(event);
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
