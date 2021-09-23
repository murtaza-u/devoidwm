#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xutil.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MODCLEAN(mask) (mask & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

typedef struct Client Client;
struct Client {
    unsigned int x, y, old_x, old_y;
    unsigned int width, height, old_width, old_height;
    Window win;
    Client *next;
    Client *prev;
    bool isfloating, isfullscreen;
};

static void start(void);
static void stop(void);
static void loop(void);
static void getInput(void);
static void handleKeyPress(XEvent *event);
static void handleButtonPress(XEvent *event);
static void handlePointerMotion(XEvent *event);
static void handleButtonRelease(XEvent *event);
static void map(XEvent *event);
static void configureWindow(Client *client);
static void quit(XEvent *event, char *command);
static void changeFocus(XEvent *event, char *command);
static void kill(XEvent *event, char *command);
static void destroyNotify(XEvent *event);
static void restack();
static void configureSlaveWindows(Client *firstSlave, unsigned int slaveCount);
static void zoom(XEvent *event, char *command);
static void swap(Client *focusedClient, Client *targetClient);
static void swapWithNeighbour(XEvent *event, char *command);
static void focus(Client *client);
static void setClientRules(Client *client);

static bool running;
static Display *dpy;
static XButtonEvent prevPointerPosition;
static XWindowAttributes attr;

struct root {
    Window win;
    int width;
    int height;
} root;

static Client *head = NULL;
static Client *tail = NULL;
static Client *focused = NULL;
static unsigned int totalClients = 0;

static const unsigned int MODKEY = Mod4Mask;

typedef struct {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(XEvent *event, char *command);
    char *command;
} key;

static const key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, NULL},
    {MODKEY, XK_j, changeFocus, "next"},
    {MODKEY, XK_k, changeFocus, "prev"},
    {MODKEY, XK_x, kill, NULL},
    {MODKEY, XK_space, zoom, NULL},
    {MODKEY|ShiftMask, XK_j, swapWithNeighbour, "next"},
    {MODKEY|ShiftMask, XK_k, swapWithNeighbour, "prev"},
};

typedef struct Rules {
    char *class;
    bool isfloating;
} Rules;

static const Rules rules[] = {
    {"Pinentry-gtk-2", true},
};

static void (*handleEvents[LASTEvent])(XEvent *event) = {
    [KeyPress] = handleKeyPress,
    [ButtonPress] = handleButtonPress,
    [ButtonRelease] = handleButtonRelease,
    [MotionNotify] = handlePointerMotion,
    [MapRequest] = map,
    [DestroyNotify] = destroyNotify,
};

void setClientRules(Client *client) {
    client -> isfloating = false;

    XClassHint hint;
    XGetClassHint(dpy, client -> win, &hint);

    for (size_t i = 0; i < sizeof(rules)/ sizeof(Rules); i ++) {
        if (strcmp(hint.res_class, rules[i].class) == 0) {
            client -> isfloating = rules[i].isfloating;
            break;
        };
    }
}

void focus(Client *client) {
    focused = client;
    XSetInputFocus(dpy, focused -> win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, focused -> win);
}

void swapWithNeighbour(XEvent *event, char *command) {
    (void)event;
    if (focused == NULL || totalClients == 1 || focused -> isfloating) return;
    if (command[0] == 'n') swap(focused, focused -> next != NULL ? focused -> next : head);
    else swap(focused, focused -> prev != NULL ? focused -> prev : tail);
}

void swap(Client *focusedClient, Client *targetClient) {
    Window temp = focusedClient -> win;
    focusedClient -> win = targetClient -> win;
    targetClient -> win = temp;

    configureWindow(focusedClient);
    configureWindow(targetClient);
    focus(targetClient);
}

void zoom(XEvent *event, char *command) {
    (void)command;
    (void)event;
    if (focused == NULL || totalClients == 1 || focused -> isfloating) return;

    swap(head, focused);
    focus(head);
}

void configureSlaveWindows(Client *firstSlave, unsigned int slaveCount) {
    for (unsigned int i = 0; i < slaveCount; i ++) {
        firstSlave -> x = root.width / 2;
        firstSlave -> y = (i * root.height) / slaveCount;
        firstSlave -> width = root.width / 2;
        firstSlave -> height = root.height / slaveCount;
        configureWindow(firstSlave);
        firstSlave = firstSlave -> next;
    }
}

void restack() {
    if (head == NULL) return;
    head -> x = 0;
    head -> y = 0;
    head -> height = root.height;
    if (totalClients == 1) head -> width = root.width;
    else {
        head -> width = root.width / 2;
        configureSlaveWindows(head -> next, totalClients - 1);
    }
    configureWindow(head);
}

void destroyNotify(XEvent *event) {
    Window destroyedWindow = event -> xdestroywindow.window;
    Client *client = head;

    while (client != NULL) {
        if (client -> win != destroyedWindow) {
            client = client -> next;
            continue;
        };

        if (client -> next != NULL) client -> next -> prev = client -> prev;
        if (client -> prev != NULL) client -> prev -> next = client -> next;
        if (client == head) head = head -> next;
        if (client == tail) tail = tail -> prev;
        break;
    }

    if (client == NULL) {
        free(focused);
        if (head != NULL) focus(head);
        return;
    }

    if (head != NULL) focus(client -> prev != NULL ? client -> prev : tail);

    totalClients --;
    free(client);
    restack();
}

void kill(XEvent *event, char *command) {
    (void)command;
    (void)event;
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, focused -> win);
}

void changeFocus(XEvent *event, char *command) {
    (void)event;
    if ((focused == NULL && focused -> next == NULL && focused -> prev == NULL)
        || focused -> isfloating)
        return;

    if (command[0] == 'n')
        focused = (focused -> next != NULL) ? focused -> next : head;
    else focused = (focused -> prev != NULL) ? focused -> prev : tail;

    focus(focused);
}

void quit(XEvent *event, char *command) {
    (void)event;
    (void)command;
    running = false;
}

void configureWindow(Client *client) {
    XWindowChanges changes = {
        .x = client -> x,
        .y = client -> y,
        .width = client -> width,
        .height = client -> height
    };
    XConfigureWindow(dpy, client -> win, CWX|CWY|CWWidth|CWHeight, &changes);
}

void map(XEvent *event) {
    Client *newClient = (Client *)malloc(sizeof(Client));
    newClient -> win = event -> xmaprequest.window;
    XMapWindow(dpy, newClient -> win);
    XSelectInput(dpy, newClient -> win, StructureNotifyMask);
    focus(newClient);

    setClientRules(newClient);
    if (newClient -> isfloating) return;

    newClient -> x = 0;
    newClient -> y = 0;
    newClient -> height = root.height;

    if (head == NULL) {
        newClient -> width = root.width;
        tail = newClient;
    } else {
        newClient -> width = root.width / 2;
        configureSlaveWindows(head, totalClients);
        head -> prev = newClient;
    }

    totalClients ++;
    newClient -> next = head;
    newClient -> prev = NULL;
    head = newClient;

    configureWindow(newClient);
}

void handleButtonPress(XEvent *event) {
    if(event -> xbutton.subwindow == None) return;
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
    bool isLeftClick = prevPointerPosition.button == 1;
    XMoveResizeWindow(
        dpy,
        event -> xmotion.window,
        attr.x + (isLeftClick ? dx : 0),
        attr.y + (isLeftClick ? dy : 0),
        MAX(5, attr.width + (isLeftClick ? 0 : dx)),
        MAX(5, attr.height + (isLeftClick ? 0 : dy))
    );
}

void handleButtonRelease(XEvent *event) {
    (void)event;
    XUngrabPointer(dpy, CurrentTime);
}

void handleKeyPress(XEvent *event) {

    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym
                && MODCLEAN(keys[i].modifier) == MODCLEAN(event -> xkey.state))
            keys[i].execute(event, keys[i].command);
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

    XGrabButton(dpy, 1, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
}

void loop(void) {
    XEvent event;
    while (running) {
        XNextEvent(dpy, &event); // blocking -> waits for next event to occur
        if (handleEvents[event.type])
            handleEvents[event.type](&event);
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
