#include <X11/X.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

static void start(void);
static void stop(void);
static void loop(void);
static void getInput(void);

static Display *dpy;
static Window root;

void getInput(void) {
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_space), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
}

void loop(void) {
    XEvent event;
    while (1) {
        XNextEvent(dpy, &event);
        if (event.type == KeyPress) printf("A key is pressed");
    }
}

void start(void) {
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Failed to establish a connection with the xserver\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Connected to the xserver\n");
    root = DefaultRootWindow(dpy);
}

void stop(void) {
    XCloseDisplay(dpy);
    fprintf(stdout, "Disconnected from the xserver\n");
}

int main(void) {
    start();
    loop();
    stop();
    return 0;
}
