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

// void tile() {
//     if (!head) return;
//
//     unsigned int n = 0, i = 0, mwidth;
//     Client *c = nexttiled(head, 0), *prev = NULL;
//
//     for (; c; c = nexttiled(c -> next, 0), n ++);
//
//     mwidth = root.width * (n > nmaster ? mratio : 1);
//
//     c = nexttiled(head, 0);
//     for (; c; c = nexttiled(c -> next, 0), i ++) {
//         if (i == 0) {
//             c -> x = root.x;
//             c -> y = root.y;
//             c -> width = mwidth;
//             c -> height = root.height;
//         } else if (i < nmaster && i == 1) {
//             prev -> height /= 2;
//             c -> y = prev -> y + prev -> height;
//             c -> x = prev -> x;
//             c -> width = prev -> width;
//             c -> height = prev -> height;
//         } else if (i == nmaster) {
//             c -> x = root.width * mratio;
//             c -> y = root.y;
//             c -> width = root.width * (1 - mratio);
//             c -> height = root.height;
//         } else shrink(prev, &c -> x, &c -> y, &c -> width, &c -> height);
//
//         if (prev) resize(prev);
//         if (i == n - 1) resize(c);
//         prev = c;
//     }
// }

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

    if (prev) resize(prev);
    if (i == n - 1) resize(c);
}

void tile() {
    if (!head) return;

    unsigned int n = 0, mwidth;

    /* calculating total clients */
    Client *c = nexttiled(head, 0);
    for (; c; c = nexttiled(c -> next, 0), n ++);

    mwidth = root.width * (n > nmaster ? mratio : 1);
    dwindle(nexttiled(head, 0), NULL, 0, n, mwidth);
}
