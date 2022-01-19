#include <X11/Xlib.h>

#include "dwindle.h"
#include "client.h"
#include "devoid.h"
#include "../config.h"

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

void dwindle(Client *c, Client *prev, unsigned int i, unsigned int n, unsigned int mwidth) {
    if (!c) return;

    if (i == 0) {
        c -> x = root.x;
        c -> y = root.y;
        c -> width = mwidth;
        c -> height = root.height;
    } else if (i < nmaster && i == 1) {
        prev -> height /= 2;
        c -> y = prev -> y + prev -> height;
        c -> x = prev -> x;
        c -> width = prev -> width;
        c -> height = prev -> height;
    } else if (i == nmaster) {
        c -> x = root.width * mratio;
        c -> y = root.y;
        c -> width = root.width * (1 - mratio);
        c -> height = root.height;
    } else shrink(prev, &c -> x, &c -> y, &c -> width, &c -> height);

    dwindle(nexttiled(c -> next, 0), c, i + 1, n, mwidth);

    if (i == n - 1) resize(c);
    if (prev) resize(prev);
}

void mirror_dwindle(Client *c, Client *prev, unsigned int i, unsigned int n, unsigned int mwidth) {
    if (!c) return;

    if (i == 0) {
        c -> x = root.x;
        c -> y = root.y;
        c -> height = mwidth;
        c -> width = root.width;
    } else if (i < nmaster && i == 1) {
        prev -> width /= 2;
        c -> x = prev -> x + prev -> width;
        c -> y = prev -> y;
        c -> height = prev -> height;
        c -> width = prev -> width;
    } else if (i == nmaster) {
        c -> y = root.height * mratio;
        c -> x = root.x;
        c -> height = root.height * (1 - mratio);
        c -> width = root.width;
    } else shrink(prev, &c -> x, &c -> y, &c -> width, &c -> height);

    mirror_dwindle(nexttiled(c -> next, 0), c, i + 1, n, mwidth);

    if (i == n - 1) resize(c);
    if (prev) resize(prev);
}

void tile() {
    if (!head) return;

    unsigned int n = 0, mwidth;

    /* calculating total clients */
    Client *c = nexttiled(head, 0);
    for (; c; c = nexttiled(c -> next, 0), n ++);

    switch (root.layout) {
        case DWINDLE:
            mwidth = root.width * (n > nmaster ? mratio : 1);
            dwindle(nexttiled(head, 0), NULL, 0, n, mwidth);
            break;

        case MIRROR_DWINDLE:
            mwidth = root.height * (n > nmaster ? mratio : 1);
            mirror_dwindle(nexttiled(head, 0), NULL, 0, n, mwidth);
            break;
    }
}

void setlayout(Arg arg) {
    if (arg.ui != root.layout) {
        root.layout = arg.ui;
        tile();
    }
}
