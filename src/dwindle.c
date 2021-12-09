#include "dwindle.h"
#include "client.h"
#include "devoid.h"
#include "../config.h"
#include <X11/Xlib.h>

void shrink(Client *c, int *x, int *y, unsigned int *width, unsigned int *height) {
    if (c -> width >= c -> height) {
        c -> width /= 2;
        *x = c -> x + c -> width;
        *y = c -> y;
    } else {
        c -> height /= 2;
        *y = c -> y + c -> height;
        *x = c -> x;
    }

    *width = c -> width;
    *height = c -> height;
}

void expand(Client *c, int x, int y) {

}

void tile() {
    if (!head) return;

    unsigned int n = 0, i = 0, mwidth;
    Client *c = nexttiled(head), *prev = NULL;

    for (; c; c = nexttiled(c -> next), n ++);
    mwidth = n > nmaster ? mratio * root.width : root.width;

    c = nexttiled(head);
    for (; c; c = nexttiled(c -> next), i ++) {
        if (i == 0) {
            c -> x = root.x;
            c -> y = root.y;
            c -> width = mwidth;
            c -> height = root.height;
        } else if (i == nmaster) {
            c -> x = root.width * mratio;
            c -> y = root.y;
            c -> width = root.width * (1 - mratio);
            c -> height = root.height;
        } else shrink(prev, &c -> x, &c -> y, &c -> width, &c -> height);

        if (prev) XMoveResizeWindow(dpy, prev -> win, prev -> x, prev -> y,
                                    prev -> width, prev -> height);

        if (i == n - 1) XMoveResizeWindow(dpy, c -> win, c -> x, c -> y,
                                          c -> width, c -> height);

        prev = c;
    }
}
