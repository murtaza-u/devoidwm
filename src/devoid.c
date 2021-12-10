#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "devoid.h"
#include "events.h"
#include "ewmh.h"
#include "key.h"
#include "mouse.h"

bool isrunning;
Display *dpy;
XWindowAttributes attr;
int screen;
Client *head, *sel;
unsigned int seltags, nmaster;
float mratio;
struct Root root;

#include "../config.h"

int main() {
    start();
    grab();
    loop();
    stop();
    return 0;
}

void start() {
    if (!(dpy = XOpenDisplay(0)))
        die("failed to open display");

    /* install a sigchl handler */
    sigchld(0);

    /* ignore all X errors */
    XSetErrorHandler(ignore);

    /* the default screen */
    screen = DefaultScreen(dpy);

    /* root window */
    root.win = DefaultRootWindow(dpy);
    root.x = 0;
    root.y = 0;
    root.width = XDisplayWidth(dpy, screen);
    root.height = XDisplayHeight(dpy, screen);

    head = sel = NULL;

    seltags = 1 << 0;

    nmaster = 1;
    mratio = 0.5;

    /* for quiting wm */
    isrunning = 1;

    /* get MapRequest events */
    XSelectInput(dpy, root.win, SubstructureRedirectMask);

    setup_ewmh_atoms();

    setup_cursor();
}

void grab() {
    for (size_t i = 0; i < sizeof(keys) / sizeof(Key); i ++)
        XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym), keys[i].modifier,
            root.win, True, GrabModeAsync, GrabModeAsync);

    XGrabButton(dpy, 1, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync,
                None, None);
    XGrabButton(dpy, 3, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync,
                None, None);
}

void loop() {
    XEvent ev;
    while (isrunning && !XNextEvent(dpy, &ev)) handle_event(&ev);
}

void stop() {
    unsigned int n;
    Window root_return, parent_return, *children;

    /* Kill every last one of them */
    if (XQueryTree(dpy, root.win, &parent_return, &root_return, &children, &n))
        for (unsigned int i = 0; i < n; i ++)
            sendevent(children[i], XInternAtom(dpy, "WM_DELETE_WINDOW", True));

    XUngrabKey(dpy, AnyKey, AnyModifier, root.win);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root.win, net_atoms[NetActiveWindow]);
    XCloseDisplay(dpy);
}

void die(char *exit_msg) {
    fprintf(stderr, "devoidwm: %s\n", exit_msg);
    exit(EXIT_FAILURE);
}

/* Taken from dwm */
void sigchld(int unused) {
    (void)unused;
    if(signal(SIGCHLD, sigchld) == SIG_ERR) die("Can't install SIGCHLD handler");
    while(0 < waitpid(-1, NULL, WNOHANG));
}

int ignore() {
    return 0;
}
