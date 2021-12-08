#include "client.h"
#include "devoid.h"
#include "events.h"

void attach(Client *client) {
    /* insert new client in circular doubly linked list */
    if (root.head == NULL) {
        root.head = client;
        root.head -> next = root.head -> prev = NULL;
    } else if (root.head -> next == NULL) {
        root.head -> next = root.head -> prev = client;
        client -> next = client -> prev = root.head;
    } else {
        root.head -> prev -> next = client;
        client -> prev = root.head -> prev;
        client -> next = root.head;
        root.head -> prev = client;
    }
}

void detach(Client *client) {
    if (client == root.head) root.head = root.head -> next;

    if (client -> next != NULL) {
        if (client -> next -> next == client) {
            client -> next -> next = NULL;
            client -> next -> prev = NULL;
        } else {
            client -> next -> prev = client -> prev;
            client -> prev -> next = client -> next;
        }
    }
}

Client* wintoclient(Window win) {
    if (root.head == NULL) return NULL;

    Client *c = root.head;
    do {
        if (c -> win == win) return c;
        c = c -> next;
    } while (c != NULL && c -> next != root.head);

    return NULL;
}

void focus(Client *c) {
    if (c == NULL) return;
    XSetInputFocus(dpy, c -> win, RevertToParent, CurrentTime);
    if (c -> isfloating) XRaiseWindow(dpy, c -> win);
    sendevent(root.focused -> win, XInternAtom(dpy, "WM_TAKE_FOCUS", False));
    root.focused = c;
}
