#include "client.h"
#include "dwindle.h"
#include "devoid.h"
#include "tags.h"
#include <X11/Xlib.h>

struct Tag tags[9];

void setup_tags() {
    for (unsigned int i = 0; i < 9; i ++)
        tags[i].focused = NULL;
}

void view(Arg arg) {
    if (arg.ui == seltags) return;
    seltags = arg.ui;
    showhide(head);
    tile();
    focus(NULL);
}

void toggletag(Arg arg) {
    seltags ^= arg.ui;
    showhide(head);
    tile();
}
