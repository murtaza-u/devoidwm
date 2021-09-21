#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

static void start(void);
static void stop(void);
static void loop(void);

static Display *dpy;

void loop(void) {
    XEvent event;
    while (1) {
        XNextEvent(dpy, &event);
    }
}

void start(void) {
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Failed to establish a connection with the xserver\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Connected to the xserver\n");
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
