#include <X11/Xlib.h>
#include <stdbool.h>

#include "client.h"
#include "dwindle.h"
#include "devoid.h"
#include "tags.h"

struct Tag tags[9];

void setup_tags() {
    for (unsigned int i = 0; i < 9; i ++) {
        tags[i].focused = NULL;
        tags[i].fullscrlock = 0;
    }
}

void view(Arg arg) {
    if (arg.ui == seltags) return;
    seltags = arg.ui;
    tile();
    showhide(head);
    focus(NULL);
    if (get_fullscrlock(seltags)) lock_fullscr(sel);
}

void toggletag(Arg arg) {
    seltags ^= arg.ui;
    tile();
    showhide(head);
}

void save_fullscrlock(Client *c) {
    for (unsigned int i = 0; i < 9; i ++)
        if ((1 << i) & c -> tags) tags[i].fullscrlock = c -> isfullscr;
}

bool get_fullscrlock(unsigned int t) {
    for (unsigned int i = 0; i < 9; i ++)
        if ((1 << i) & t) return tags[i].fullscrlock;
    return 0;
}
