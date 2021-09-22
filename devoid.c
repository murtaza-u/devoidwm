#include <X11/X.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)

typedef struct Client Client;
struct Client {
    unsigned int x, y, old_x, old_y;
    unsigned int width, height, old_width, old_height;
    Window win;
    Client *next;
    Client *prev;
};

static void start(void);
static void stop(void);
static void loop(void);
static void getInput(void);
static void handleKeyPress(XEvent *event);
static void handleMouseClick(XEvent *event);
static void handlePointerMotion(XEvent *event);
static void map(XEvent *event);
static void configureWindow(Client *client);
static void quit(XEvent *event, char *command);
static void changeFocus(XEvent *event, char *command);
static void kill(XEvent *event, char *command);
static void destroyNotify(XEvent *event);
static void restack();

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

static Client *head = NULL;
static Client *tail = NULL;
static Client *focused = NULL;
static unsigned int totalClients = 0;

static const unsigned int MODKEY = Mod4Mask;

static const key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, NULL},
    {MODKEY, XK_j, changeFocus, "next"},
    {MODKEY, XK_k, changeFocus, "prev"},
    {MODKEY, XK_x, kill, NULL},
};

void restack() {
    if (head == NULL) return;

    head -> x = 0;
    head -> y = 0;
    head -> height = root.height;

    if (totalClients == 1) head -> width = root.width;
    else head -> width = root.width / 2;

    configureWindow(head);

    if (totalClients == 1) return;

    Client *client = head -> next;
    for (unsigned int i = 0; i < totalClients - 1; i ++) {
        client -> x = root.width / 2;
        client -> y = (i * root.height) / (totalClients - 1);
        client -> width = root.width / 2;
        client -> height = root.height / (totalClients - 1);
        configureWindow(client);
        client = client -> next;
    }
}

void destroyNotify(XEvent *event) {
    Window destroyedWindow = event -> xdestroywindow.window;

    if (head -> win == destroyedWindow) {
        Client *temp = head;
        head = head -> next;
        if (head != NULL) head -> prev = NULL;
        else tail = NULL;
        free(temp);
    } else if (tail -> win == destroyedWindow) {
        Client *temp = tail;
        tail = tail -> prev;
        if (tail != NULL) tail -> next = NULL;
        else head = NULL;
        free(temp);
    } else {
        Client *client = head -> next;
        while (client != tail && client != NULL) {
            if (client -> win == destroyedWindow) {
                if (client -> next != NULL) client -> next -> prev = client -> prev;
                if (client -> prev != NULL) client -> prev -> next = client -> next;
                free(client);
                break;
            }
            client = client -> next;
        }
    }

    focused = head;
    if (focused != NULL) {
        XSetInputFocus(dpy, focused -> win, RevertToParent, CurrentTime);
        XRaiseWindow(dpy, focused -> win);
    }

    totalClients --;
    restack();
}

void kill(XEvent *event, char *command) {
    XDestroyWindow(dpy, event -> xkey.subwindow);
}

void changeFocus(XEvent *event, char *command) {
    if (focused == NULL && focused -> next == NULL && focused -> prev == NULL)
        return;

    if (command[0] == 'n')
        focused = (focused -> next != NULL) ? focused -> next : head;
    else if (command[0] == 'p')
        focused = (focused -> prev != NULL) ? focused -> prev : tail;

    XSetInputFocus(dpy, focused -> win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, focused -> win);
}

void quit(XEvent *event, char *command) {
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
    newClient -> x = 0;
    newClient -> y = 0;
    newClient -> height = root.height;

    if (head == NULL) {
        newClient -> width = root.width;
        tail = newClient;
    } else {
        newClient -> width = root.width / 2;
        Client *client = head;
        for (unsigned int i = 0; i < totalClients; i ++) {
            client -> x = root.width / 2;
            client -> y = (i * root.height) / totalClients;
            client -> width = root.width / 2;
            client -> height = root.height / totalClients;
            configureWindow(client);
            client = client -> next;
        }
        head -> prev = newClient;
    }

    totalClients ++;
    newClient -> next = head;
    newClient -> prev = NULL;
    head = newClient;

    configureWindow(newClient);
    XMapWindow(dpy, newClient -> win);
    XSelectInput(dpy, newClient -> win, StructureNotifyMask);
    XSetInputFocus(dpy, newClient -> win, RevertToParent, CurrentTime);
    focused = newClient;
    XRaiseWindow(dpy, newClient -> win);
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

    XGrabButton(dpy, 1, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
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
        else if (event.type == DestroyNotify)
            destroyNotify(&event);
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
