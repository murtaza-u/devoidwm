#include "dwindle.h"
#include "devoid.h"

void view(Arg arg) {
    if (arg.ui == seltags) return;
    seltags = arg.ui;
    showhide(head);
    tile();
}

void toggletag(Arg arg) {
    seltags ^= arg.ui;
    showhide(head);
    tile();
}
