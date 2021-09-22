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

static void print(char *command, XEvent *event) {
    printf("%s\n", command);
}

static Display *dpy;
static Window root;

typedef struct {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(char *command, XEvent *event);
    char *command;
} key;

static const unsigned int MODKEY = Mod4Mask;

static const key keys[] = {
    {MODKEY, XK_space, print, "Hello, world"},
};

void getInput(void) {
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_space), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
}

void loop(void) {
    XEvent event;
    while (1) {
        XNextEvent(dpy, &event);
        fflush(stdout);
        if (event.type == KeyPress) fprintf(stdout, "A key is pressed\n");
        else if(event.type == ButtonPress) fprintf(stdout, "Mouse Left click\n");
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
    getInput();
    loop();
    stop();
    return 0;
}