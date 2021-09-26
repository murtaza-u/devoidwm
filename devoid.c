#include <X11/X.h>
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

#define MOVERESIZE(win, x, y, width, height) XMoveResizeWindow(dpy, win, x, y, width, height)

typedef struct Client Client;
struct Client {
    int x, y;
    unsigned int width, height;
    Client *next, *prev;
    Window win;
};

static Client *head, *focused;
static unsigned int total_clients;
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
static void keypress(XEvent *event);
static void buttonpress(XEvent *event);
static void maprequest(XEvent *event);
static void configurerequest(XEvent *event);
static void destroynotify(XEvent *event);

static void quit(XEvent *event, char *command);
static void add_client(Window win);
static void tile();
static void tile_master();
static void tile_slaves();
static void focus(Client *client);

static void (*event_handler[LASTEvent])(XEvent *event) = {
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


// function implementation
void focus(Client *client) {
    if (!client) client = head;
    focused = client;
    if (focused == NULL) return;
    XSetInputFocus(dpy, focused -> win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, focused -> win);
}

void tile_master() {
    if (head == NULL) return;
    Client *master = head -> next == NULL ? head : head -> next;
    master -> x = 0;
    master -> y = 0;
    master -> height = root.height;
    master -> width = master -> next != NULL ? root.width/2 : root.width;
    MOVERESIZE(master -> win, master -> x, master -> y, master -> width, master -> height);
}

void tile_slaves() {
    if (head == NULL || head -> next == NULL) return;
    Client *slave = head -> next -> next;
    unsigned int slave_count = total_clients - 1;
    for (unsigned int i = 0; i < slave_count; i ++, slave = slave -> next) {
        slave -> x = root.width / 2;
        slave -> y = (i * root.height) / slave_count;
        slave -> width = root.width / 2;
        slave -> height = root.height / slave_count;
        MOVERESIZE(slave -> win, slave -> x, slave -> y, slave -> width, slave -> height);
    }
}

void tile() {
    tile_master();
    tile_slaves();
}

void add_client(Window win) {
    Client *new_client = (Client *)malloc(sizeof(Client));
    new_client -> win = win;
    new_client -> prev = head;
    if (head == NULL) new_client -> next = NULL;
    else if (head -> next == NULL) {
        new_client -> next = head;
        head -> next = head -> prev = new_client;
    } else {
        new_client -> next = head -> next;
        head -> next = new_client;
        head -> next -> prev = new_client;
    }

    head = new_client;
    total_clients ++;
}

void destroynotify(XEvent *event) {

}

void configurerequest(XEvent *event) {

}

void maprequest(XEvent *event) {
    XMapRequestEvent *ev = &event -> xmaprequest;
    add_client(ev -> window);
    XMapWindow(dpy, ev -> window);
    tile();
    focus(0);
}

void buttonpress(XEvent *event) {

}

void keypress(XEvent *event) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym &&
                MODCLEAN(keys[i].modifier) == MODCLEAN(event -> xkey.state))
            keys[i].execute(event, keys[i].command);
    }
}

void quit(XEvent *event, char *command) {
    (void)event;
    (void)command;
    isrunning = false;
}

void loop() {
    XEvent ev;

    // The main loop.
    // XNextEvent is a blocking call
    while (isrunning && !XNextEvent(dpy, &ev)) { if (event_handler[ev.type])
            event_handler[ev.type](&ev);
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
    (void)unused;
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
    total_clients = 0;

    // initialise workspaces
    for (int i = 0; i < MAX_WORKSPACES; i ++) {
        workspaces[i].head = NULL;
        workspaces[i].focused = NULL;
    }

    current_ws = 0;

    XSelectInput(dpy, root.win, SubstructureNotifyMask|SubstructureRedirectMask);
	XDefineCursor(dpy, root.win, XCreateFontCursor(dpy, 68));
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
