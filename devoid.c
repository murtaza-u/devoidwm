#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
static void handleMouseClick(XEvent *event);
static void handlePointerMotion(XEvent *event);
static void map(XEvent *event);
static void quit(XEvent *event, char *command);

static bool running;
static Display *dpy;
static XButtonEvent prevPointerPosition;
static XWindowAttributes attr;

struct root {
    Window win;
    int width;
    int height;
} root;

typedef struct {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(XEvent *event, char *command);
    char *command;
} key;

typedef struct {
    int x, y, old_x, old_y;
    int width, height, old_width, old_height;
    Window win;
    struct client *next;
    struct client *prev;
} client;

static client *head = NULL;
static const unsigned int MODKEY = Mod4Mask;

static const key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, NULL},
};

void quit(XEvent *event, char *command) {
    running = false;
}

void map(XEvent *event) {
    Window window = event -> xmaprequest.window;
    XMoveResizeWindow(dpy, window, 0, 0, root.width, root.height);
    XMapWindow(dpy, window);
}

void handleMouseClick(XEvent *event) {
    XGrabPointer(
        dpy,
        event -> xbutton.subwindow,
        True,
        PointerMotionMask|ButtonReleaseMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime
    );
    XGetWindowAttributes(dpy, event -> xbutton.subwindow, &attr);
    prevPointerPosition = event -> xbutton;
}

void handlePointerMotion(XEvent *event) {
    while(XCheckTypedEvent(dpy, MotionNotify, event));
    int dx = event -> xbutton.x_root - prevPointerPosition.x_root;
    int dy = event -> xbutton.y_root - prevPointerPosition.y_root;
    int isLeftClick = prevPointerPosition.button == 1;
    XMoveResizeWindow(
        dpy,
        event -> xmotion.window,
        attr.x + (isLeftClick ? dx : 0),
        attr.y + (isLeftClick ? dy : 0),
        MAX(5, attr.width + (isLeftClick ? 0 : dx)),
        MAX(5, attr.height + (isLeftClick ? 0 : dy))
    );
}

void handleKeyPress(XEvent *event) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym) {
            keys[i].execute(event, keys[i].command);
        }
    }
}

void getInput(void) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++)
        XGrabKey(
            dpy,
            XKeysymToKeycode(dpy, keys[i].keysym),
            keys[i].modifier,
            root.win,
            True,
            GrabModeAsync,
            GrabModeAsync
        );

    XGrabButton(dpy, 1, Mod1Mask, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
}

void loop(void) {
    XEvent event;
    while (running) {
        XNextEvent(dpy, &event); // blocking -> waits for next event to occur
        if (event.type == KeyPress)
            handleKeyPress(&event);
        else if(event.type == ButtonPress && event.xbutton.subwindow != None)
            handleMouseClick(&event);
        else if (event.type == MotionNotify)
            handlePointerMotion(&event);
        else if (event.type == ButtonRelease)
            XUngrabPointer(dpy, CurrentTime);
        else if (event.type == MapRequest)
            map(&event);
    }
}

void start(void) {
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Failed to establish a connection with the xserver\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Connected to the xserver\n");
    running = true;

    // root window
    root.win = DefaultRootWindow(dpy);
    XGetWindowAttributes(dpy, root.win, &attr);
    root.width = attr.width;
    root.height = attr.height;

    XSelectInput(dpy, root.win, SubstructureRedirectMask);
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
