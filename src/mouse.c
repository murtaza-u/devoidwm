#include <X11/X.h>
#include <X11/Xlib.h>

#include "client.h"
#include "devoid.h"
#include "mouse.h"
#include "dwindle.h"

XButtonEvent prev_pointer_position;
Cursor cursors[CurLast];

void setup_cursor() {
    /* set cursors */
    cursors[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
    cursors[CurResize] = XCreateFontCursor(dpy, XC_sizing);
    cursors[CurMove] = XCreateFontCursor(dpy, XC_fleur);

    /* define the cursor */
    XDefineCursor(dpy, root.win, cursors[CurNormal]);
}

void handle_buttonpress(XEvent *event) {
    if(event -> xbutton.subwindow == None ||
        (event -> xbutton.button == 1 && event -> xbutton.button == 3)) return;

    if (XGrabPointer(dpy, event -> xbutton.subwindow, True, PointerMotionMask|ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None,
                     event -> xbutton.button == 1 ? cursors[CurMove] : cursors[CurResize],
                     CurrentTime) != GrabSuccess)
        return;

    XGetWindowAttributes(dpy, event -> xbutton.subwindow, &attr);
    prev_pointer_position = event -> xbutton;
}

void handle_motionnotify(XEvent *event) {
    while(XCheckTypedEvent(dpy, MotionNotify, event));
    int dx = event -> xbutton.x_root - prev_pointer_position.x_root;
    int dy = event -> xbutton.y_root - prev_pointer_position.y_root;
    bool isLeftClick = prev_pointer_position.button == 1;

    if (!sel -> isfloating) {
        sel -> isfloating = 1;
        tile();
    }

    sel -> x = attr.x + (isLeftClick ? dx : 0);
    sel -> y = attr.y + (isLeftClick ? dy : 0);
    sel -> width = MAX(1, attr.width + (isLeftClick ? 0 : dx));
    sel -> height = MAX(1, attr.height + (isLeftClick ? 0 : dy));

    resize(sel);
}
