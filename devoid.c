#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#define MAX_WORKSPACES 9
#define MODCLEAN(mask) (mask & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

typedef struct Client Client;
struct Client {
    int x, y;
    unsigned int width, height;
    Client *next, *prev;
};

static Client *head, *focused;
static bool isrunning;
static Display *dpy;
static int screen;

// The root window
struct {
    Window win;
    unsigned int width, height; // same as the width and height of the screen
} root;

// virtual desktops/workspaces
struct {
    Client *head, *focused;
} workspaces[MAX_WORKSPACES];

static int current_ws;

static void die(char *exit_message);
static void sigchld(int unused);
static void setup();
static void grab();
static void loop();

// events
static void keypress(XEvent *ev);
static void buttonpress(XEvent *ev);
static void maprequest(XEvent *ev);
static void configurerequest(XEvent *ev);
static void destroynotify(XEvent *ev);

static void quit(XEvent *ev, char *command);

static void (*event_handler[LASTEvent])(XEvent *ev) = {
    [KeyPress] = keypress,
    [ButtonPress] = buttonpress,
    [MapRequest] = maprequest,
    [ConfigureRequest] = configurerequest,
    [DestroyNotify] = destroynotify
};

static const unsigned int MODKEY = Mod4Mask;

typedef struct {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(XEvent *event, char *command);
    char *command;
} key;

static const key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, NULL},
};

void keypress(XEvent *ev) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, ev -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym &&
                MODCLEAN(keys[i].modifier) == MODCLEAN(ev -> xkey.state))
            keys[i].execute(ev, keys[i].command);
    }
}

void quit(XEvent *ev, char *command) {
    isrunning = false;
}

void loop() {
    XEvent ev;

    // The main loop.
    // XNextEvent is a blocking call
    while (isrunning && !XNextEvent(dpy, &ev)) {
        // listen to events
    }
}

void grab() {
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
}

// Taken from dwm
void sigchld(int unused) {
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("couldn't install SIGCHLD handler");
    while (0 < waitpid(-1, NULL, WNOHANG));
}

void setup() {
    sigchld(0);

    if (!(dpy = XOpenDisplay(0)))
        die("could not open display");

    screen = DefaultScreen(dpy);
    root.win = DefaultRootWindow(dpy);
    root.width = XDisplayWidth(dpy, screen);
    root.height = XDisplayHeight(dpy, screen);

    isrunning = true;

    head = focused = NULL;

    // initialise workspaces
    for (int i = 0; i < MAX_WORKSPACES; i ++) {
        workspaces[i].head = NULL;
        workspaces[i].focused = NULL;
    }

    current_ws = 0;

    XSelectInput(dpy, root.win, SubstructureNotifyMask|StructureNotifyMask);
}

void die(char *exit_message) {
    fprintf(stderr, "devoidwm: %s\n", exit_message);
    exit(EXIT_FAILURE);
}

int main() {
    setup();
    grab();
    loop();
    XCloseDisplay(dpy); // close the display
    return 0;
}
