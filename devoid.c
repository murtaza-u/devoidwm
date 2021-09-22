#include <X11/X.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)

static void start(void);
static void stop(void);
static void loop(void);
static void getInput(void);
static void handleKeyPress(XEvent *event);

static void print(char *command, XEvent *event) {
    printf("%s\n", command);
}

static Display *dpy;
static Window root;
static XButtonEvent prevPosition;
static XWindowAttributes attr;

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

void handleKeyPress(XEvent *event) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym) {
            keys[i].execute(keys[i].command, event);
        }
    }
}

void getInput(void) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        XGrabKey(
            dpy,
            XKeysymToKeycode(dpy, keys[i].keysym),
            keys[i].modifier,
            root,
            True,
            GrabModeAsync,
            GrabModeAsync
        );
    }

    XGrabButton(dpy, 1, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
}

void loop(void) {
    XEvent event;
    while (1) {
        XNextEvent(dpy, &event);
        fflush(stdout);
        if (event.type == KeyPress) handleKeyPress(&event);
        else if(event.type == ButtonPress && event.xbutton.subwindow != None) {
            XGrabPointer(
                dpy,
                event.xbutton.subwindow,
                True,
                PointerMotionMask|ButtonReleaseMask,
                GrabModeAsync,
                GrabModeAsync,
                None,
                None,
                CurrentTime
            );
            XGetWindowAttributes(dpy, event.xbutton.subwindow, &attr);
            prevPosition = event.xbutton;
        } else if (event.type == MotionNotify) {
            while(XCheckTypedEvent(dpy, MotionNotify, &event));
            int dx = event.xbutton.x_root - prevPosition.x_root;
            int dy = event.xbutton.y_root - prevPosition.y_root;
            int isLeftClick = prevPosition.button == 1;
            XMoveResizeWindow(
                dpy,
                event.xmotion.window,
                attr.x + (isLeftClick ? dx : 0),
                attr.y + (isLeftClick ? dy : 0),
                MAX(5, attr.width + (isLeftClick ? 0 : dx)),
                MAX(5, attr.height + (isLeftClick ? 0 : dy))
            );
        } else if (event.type == ButtonRelease)
            XUngrabPointer(dpy, CurrentTime);
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
