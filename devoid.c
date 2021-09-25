#include <X11/X.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MODCLEAN(mask) (mask & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define MAX_WORKSPACES 9

typedef struct Client Client;
struct Client {
    unsigned int x, y, old_x, old_y, width, height, old_width, old_height;
    Window win;
    Client *next, *prev;
    bool isfloating, isfullscreen;
};

static void start(void);
static void stop(void);
static void loop(void);
static void grab(void);

// events
static void keypress(XEvent *event);
static void buttonpress(XEvent *event);
static void pointermotion(XEvent *event);
static void buttonrelease(XEvent *event);
static void map(XEvent *event);
static void destroy_notify(XEvent *event);

static void set_client_rules(Client *client);
static void restack();
static void configure_window(Client *client);
static void configure_slave_windows(Client *first_slave, unsigned int slave_count);
static void focus_adjacent(XEvent *event, char *command);
static void focus(Client *client);
static void quit(XEvent *event, char *command);
static void kill(XEvent *event, char *command);
static void swap(Client *focused_client, Client *target_client);
static void swap_with_neighbour(XEvent *event, char *command);
static void zoom(XEvent *event, char *command);
static Atom get_atom_prop(Window win, Atom atom);
static void change_atom_property(Atom prop, unsigned char *data);

// workspaces
static void switch_workspace(XEvent *event, char *command);
static void save_workspace(Client *focused, Client *head, unsigned int total_clients, unsigned int ws);
static void ewmh_set_current_desktop(unsigned int ws);

struct {
    Client *head, *focused;
    unsigned int total_clients;
} workspaces[MAX_WORKSPACES];

// EWMH atoms
enum {
    NetSupported,
    NetCurrentDesktop,
    NetNumberOfDesktops,
    NetWMWindowType,
    NetWMWindowTypeDialog,
    NetWMWindowTypeMenu,
    NetWMWindowTypeSplash,
    NetWMWindowTypeToolbar,
    NetWMWindowTypeUtility,
    NetWMState,
    NetWMStateFullscreen,
    NetWMStateAbove,
    NetActiveWindow,
    NetLast
};

static Atom net_atoms[NetLast];

static bool running;
static Display *dpy;
static XButtonEvent prev_pointer_position;
static XWindowAttributes attr;

struct root {
    Window win;
    int width, height;
} root;

static Client *head, *focused;
static unsigned int current_ws, total_clients;

static const unsigned int MODKEY = Mod4Mask;
typedef struct {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(XEvent *event, char *command);
    char *command;
} key;

static const key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, NULL},
    {MODKEY, XK_j, focus_adjacent, "next"},
    {MODKEY, XK_k, focus_adjacent, "prev"},
    {MODKEY, XK_x, kill, NULL},
    {MODKEY, XK_space, zoom, NULL},
    {MODKEY|ShiftMask, XK_j, swap_with_neighbour, "next"},
    {MODKEY|ShiftMask, XK_k, swap_with_neighbour, "prev"},
    {MODKEY, XK_1, switch_workspace, "0"},
    {MODKEY, XK_2, switch_workspace, "1"},
    {MODKEY, XK_3, switch_workspace, "2"},
    {MODKEY, XK_4, switch_workspace, "3"},
    {MODKEY, XK_5, switch_workspace, "4"},
    {MODKEY, XK_6, switch_workspace, "5"},
    {MODKEY, XK_7, switch_workspace, "6"},
    {MODKEY, XK_8, switch_workspace, "7"},
    {MODKEY, XK_9, switch_workspace, "8"},
};

typedef struct Rules {
    char *class;
    bool isfloating;
} Rules;

static const Rules rules[] = {
    {"Gcolor3", true},
};

static void (*handle_events[LASTEvent])(XEvent *event) = {
    [KeyPress] = keypress,
    [ButtonPress] = buttonpress,
    [ButtonRelease] = buttonrelease,
    [MotionNotify] = pointermotion,
    [MapRequest] = map,
    [DestroyNotify] = destroy_notify,
};

void ewmh_set_current_desktop(unsigned int ws) {
    unsigned long data[1];
    data[0] = ws + 1;
    change_atom_property(net_atoms[NetCurrentDesktop], (unsigned char *)data);
}

void save_workspace(Client *focused, Client *head,unsigned int total_clients, unsigned int ws) {
    workspaces[ws].focused = focused;
    workspaces[ws].head = head;
    workspaces[ws].total_clients = total_clients;
}

void switch_workspace(XEvent *event, char *command) {
    (void)event;
    unsigned int ws = (int)(command[0] - '0');
    if (ws == current_ws) return;

    save_workspace(focused, head, total_clients, current_ws);

    current_ws = ws;
    Client *client;

    if (focused != NULL) {
        client = focused;
        do {
            XUnmapWindow(dpy, client -> win);
            client = client -> next;
        } while (client != NULL && client != focused);
    }

    focused = workspaces[ws].focused;
    head = workspaces[ws].head;
    total_clients = workspaces[ws].total_clients;

    if (focused != NULL) {
        client = focused;
        do {
            XMapWindow(dpy, client -> win);
            client = client -> next;
        } while (client != NULL && client != focused);
    }
    focus(focused);
    ewmh_set_current_desktop(ws);
}

void change_atom_property(Atom prop, unsigned char *data) {
    XChangeProperty(
        dpy,
        root.win,
        prop,
        XA_CARDINAL,
        32,
        PropModeReplace,
        data,
        1
    );
}

Atom get_atom_prop(Window win, Atom atom) {
    Atom prop, da;
    unsigned char *prop_ret = NULL;
    int di;
    unsigned long dl;
    if (XGetWindowProperty(
            dpy,
            win,
            atom,
            0,
            1,
            False,
            XA_ATOM,
            &da,
            &di,
            &dl,
            &dl,
            &prop_ret) == Success) {
        if (prop_ret) {
            prop = ((Atom *)prop_ret)[0];
            XFree(prop_ret);
        }
    }
    return prop;
}

void set_client_rules(Client *client) {
    XClassHint hint;
    XGetClassHint(dpy, client -> win, &hint);
    client -> isfloating = false;

    for (size_t i = 0; i < sizeof(rules)/ sizeof(Rules); i ++) {
        if (strcmp(hint.res_class, rules[i].class) == 0) {
            client -> isfloating = rules[i].isfloating;
            return;
        };
    }

    Atom prop = get_atom_prop(client -> win, net_atoms[NetWMWindowType]);
    if (prop == net_atoms[NetWMWindowTypeDialog] ||
            prop == net_atoms[NetWMWindowTypeMenu] ||
            prop == net_atoms[NetWMWindowTypeSplash] ||
            prop == net_atoms[NetWMWindowTypeToolbar] ||
            prop == net_atoms[NetWMWindowTypeUtility]) {
        client -> isfloating = true;
        return;
    }

    prop = get_atom_prop(client -> win, net_atoms[NetWMState]);
    if (prop == net_atoms[NetWMStateAbove]) {
        client -> isfloating = true;
        return;
    }
}

void focus(Client *client) {
    focused = client;
    if (focused == NULL) return;
    XSetInputFocus(dpy, focused -> win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, focused -> win);
    change_atom_property(net_atoms[NetActiveWindow], (unsigned char *)&(client -> win));
}

void swap_with_neighbour(XEvent *event, char *command) {
    (void)event;
    if (focused == NULL || focused -> next == NULL || focused -> isfloating)
        return;
    swap(focused, command[0] == 'n' ? focused -> next : focused -> prev);
}

void swap(Client *focused_client, Client *target_client) {
    Window temp = focused_client -> win;
    focused_client -> win = target_client -> win;
    target_client -> win = temp;

    configure_window(focused_client);
    configure_window(target_client);
    focus(target_client);
}

void zoom(XEvent *event, char *command) {
    (void)command;
    (void)event;
    if (focused == NULL || focused -> next == NULL || focused -> isfloating)
        return;
    swap(head, focused);
    focus(head);
}

void configure_slave_windows(Client *first_slave, unsigned int slave_count) {
    for (unsigned int i = 0; i < slave_count; i ++) {
        first_slave -> x = root.width / 2;
        first_slave -> y = (i * root.height) / slave_count;
        first_slave -> width = root.width / 2;
        first_slave -> height = root.height / slave_count;
        configure_window(first_slave);
        first_slave = first_slave -> next;
    }
}

void restack() {
    if (head == NULL) return;
    head -> x = 0;
    head -> y = 0;
    head -> height = root.height;
    if (total_clients == 1) head -> width = root.width;
    else {
        head -> width = root.width / 2;
        configure_slave_windows(head -> next, total_clients - 1);
    }
    configure_window(head);
}

void destroy_notify(XEvent *event) {
    Window destroyed_window = event -> xdestroywindow.window;
    Client *client = head;
    bool isTiled = false;

    do {
        if (client == NULL) break;
        if (client -> win == destroyed_window) {
            isTiled = true;
            if (client == head) head = head -> next;
            if (client -> next == NULL) break;
            if (client -> next -> next == client) {
                client -> next -> next = NULL;
                client -> next -> prev = NULL;
            } else {
                client -> prev -> next = client -> next;
                client -> next -> prev = client -> prev;
            }
            break;
        }
        client = client -> next;
    } while (client != head);

    if (!isTiled) {
        if (client != NULL) free(focused);
        focus(head);
        return;
    }

    if (client == NULL) {
        focused = NULL;
        return;
    };

    focus(client -> prev);
    free(client);
    total_clients --;
    restack();
}

void kill(XEvent *event, char *command) {
    (void)command;
    (void)event;
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, focused -> win);
}

void focus_adjacent(XEvent *event, char *command) {
    (void)event;
    if (focused == NULL || focused -> next == NULL || focused -> isfloating)
        return;

    focused = (command[0] == 'n') ? focused -> next : focused -> prev;
    focus(focused);
}

void quit(XEvent *event, char *command) {
    (void)event;
    (void)command;
    running = false;
}

void configure_window(Client *client) {
    XWindowChanges changes = {
        .x = client -> x,
        .y = client -> y,
        .width = client -> width,
        .height = client -> height
    };
    XConfigureWindow(dpy, client -> win, CWX|CWY|CWWidth|CWHeight, &changes);
}

void map(XEvent *event) {
    Client *new_client = (Client *)malloc(sizeof(Client));
    new_client -> win = event -> xmaprequest.window;
    XMapWindow(dpy, new_client -> win);
    XSelectInput(dpy, new_client -> win, StructureNotifyMask);
    focus(new_client);

    set_client_rules(new_client);
    if (new_client -> isfloating) {
        XGetWindowAttributes(dpy, new_client -> win, &attr);
        new_client -> width = attr.width;
        new_client -> height = attr.height;
        new_client -> x = root.width/2 - new_client -> width/2;
        new_client -> y = root.height/2 - new_client -> height/2;
        configure_window(new_client);
        return;
    };

    new_client -> x = 0;
    new_client -> y = 0;
    new_client -> height = root.height;
    new_client -> next = head;

    if (head == NULL) {
        new_client -> prev = NULL;
        new_client -> width = root.width;
    } else {
        configure_slave_windows(head, total_clients);
        new_client -> width = root.width / 2;
        if (head -> prev == NULL) {
            new_client -> prev = head;
            head -> next = new_client;
            head -> prev = new_client;
        } else {
            new_client -> prev = head -> prev;
            head -> prev -> next = new_client;
            head -> prev = new_client;
        }
    }

    head = new_client;
    total_clients ++;
    configure_window(new_client);
}

void buttonpress(XEvent *event) {
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
    prev_pointer_position = event -> xbutton;
}

void pointermotion(XEvent *event) {
    while(XCheckTypedEvent(dpy, MotionNotify, event));
    int dx = event -> xbutton.x_root - prev_pointer_position.x_root;
    int dy = event -> xbutton.y_root - prev_pointer_position.y_root;
    bool isLeftClick = prev_pointer_position.button == 1;
    XMoveResizeWindow(
        dpy,
        event -> xmotion.window,
        attr.x + (isLeftClick ? dx : 0),
        attr.y + (isLeftClick ? dy : 0),
        MAX(5, attr.width + (isLeftClick ? 0 : dx)),
        MAX(5, attr.height + (isLeftClick ? 0 : dy))
    );
}

void buttonrelease(XEvent *event) {
    (void)event;
    XUngrabPointer(dpy, CurrentTime);
}

void keypress(XEvent *event) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym
                && MODCLEAN(keys[i].modifier) == MODCLEAN(event -> xkey.state))
            keys[i].execute(event, keys[i].command);
    }
}

void grab(void) {
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
        if (handle_events[event.type])
            handle_events[event.type](&event);
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
	XDefineCursor(dpy, root.win, XCreateFontCursor(dpy, 68));

    head = focused = NULL;
    current_ws = total_clients = 0;

    for (unsigned int i = 0; i < MAX_WORKSPACES; i ++) {
        workspaces[i].head = NULL;
        workspaces[i].focused= NULL;
        workspaces[i].total_clients = 0;
    }

	net_atoms[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
    net_atoms[NetNumberOfDesktops] = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	net_atoms[NetCurrentDesktop] = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    net_atoms[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
    net_atoms[NetWMStateFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    net_atoms[NetWMStateAbove] = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    net_atoms[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    net_atoms[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    net_atoms[NetWMWindowTypeMenu] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    net_atoms[NetWMWindowTypeSplash] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    net_atoms[NetWMWindowTypeToolbar] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    net_atoms[NetWMWindowTypeUtility] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
	net_atoms[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

    change_atom_property(net_atoms[NetSupported], (unsigned char *)net_atoms);
    ewmh_set_current_desktop(0);

    unsigned long data[1];
    data[0] = MAX_WORKSPACES;
    change_atom_property(net_atoms[NetNumberOfDesktops], (unsigned char*)data);
}

void stop(void) {
    XCloseDisplay(dpy);
    fprintf(stdout, "Disconnected from the xserver\n");
}

int main(void) {
    start();
    grab();
    loop();
    stop();
    return 0;
}
