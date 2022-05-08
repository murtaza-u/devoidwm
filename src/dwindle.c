#include <X11/Xlib.h>

#include "dwindle.h"
#include "client.h"
#include "devoid.h"
#include "../config.h"

void shrink(Client *c, int *x, int *y, unsigned int *w, unsigned int *h) {
    if (c -> width >= c -> height) {
        c -> width /= 2;
        *x = c -> x + c -> width;
        *y = c -> y;
    } else {
        c -> height /= 2;
        *y = c -> y + c -> height;
        *x = c -> x;
    }

    *w = c -> width;
    *h = c -> height;
}

void dwindle(Client *c, Client *prev, unsigned int i, unsigned int n, unsigned int mw) {
    if (!c) return;

    if (i == 0) {
        c -> x = root.x;
        c -> y = root.y;
        c -> width = mw;
        c -> height = root.h;
    } else if (i < nmaster && i == 1) {
        prev -> height /= 2;
        c -> y = prev -> y + prev -> height;
        c -> x = prev -> x;
        c -> width = prev -> width;
        c -> height = prev -> height;
    } else if (i == nmaster) {
        c -> x = root.w * mratio;
        c -> y = root.y;
        c -> width = root.w * (1 - mratio);
        c -> height = root.h;
    } else shrink(prev, &c -> x, &c -> y, &c -> width, &c -> height);

    dwindle(nexttiled(c -> next, 0), c, i + 1, n, mw);

    if (i == n - 1) resize(c);
    if (prev) resize(prev);
}

void mirror_dwindle(Client *c, Client *prev, unsigned int i, unsigned int n, unsigned int mw) {
    if (!c) return;

    if (i == 0) {
        c -> x = root.x;
        c -> y = root.y;
        c -> height = mw;
        c -> width = root.w;
    } else if (i < nmaster && i == 1) {
        prev -> width /= 2;
        c -> x = prev -> x + prev -> width;
        c -> y = prev -> y;
        c -> height = prev -> height;
        c -> width = prev -> width;
    } else if (i == nmaster) {
        c -> y = root.h * mratio;
        c -> x = root.x;
        c -> height = root.h * (1 - mratio);
        c -> width = root.w;
    } else shrink(prev, &c -> x, &c -> y, &c -> width, &c -> height);

    mirror_dwindle(nexttiled(c -> next, 0), c, i + 1, n, mw);

    if (i == n - 1) resize(c);
    if (prev) resize(prev);
}

void tile() {
    if (!head) return;

    unsigned int n = 0, mw;

    /* calculating total clients */
    Client *c = nexttiled(head, 0);
    for (; c; c = nexttiled(c -> next, 0), n ++);

    switch (root.layout) {
        case DWINDLE:
            mw = root.w * (n > nmaster ? mratio : 1);
            dwindle(nexttiled(head, 0), NULL, 0, n, mw);
            break;

        case MIRROR_DWINDLE:
            mw = root.h * (n > nmaster ? mratio : 1);
            mirror_dwindle(nexttiled(head, 0), NULL, 0, n, mw);
            break;
    }
}

void setlayout(Arg arg) {
    if (arg.ui != root.layout) {
        root.layout = arg.ui;
        tile();
    }
}
