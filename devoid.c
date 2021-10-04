#include <X11/X.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)

#define MODCLEAN(mask) (mask & \
    (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

#define MOVERESIZE(win, x, y, width, height) \
    XMoveResizeWindow(dpy, win, x, y, width, height)

#define CHANGEATOMPROP(prop, type, data, nelments) \
    XChangeProperty(dpy, root.win, prop, type, 32, PropModeReplace, data, nelments);

#define GETATOMIDENTIFIER(name) XInternAtom(dpy, name, False)

typedef struct Client Client;
struct Client {
    int x, y;
    unsigned int width, height;
    Client *next, *prev;
    Window win;
    bool isfullscreen, isfloating;
};

struct {
    Window win;
    unsigned int width, height;
} root;

typedef union Arg Arg;
union Arg {
    const int i;
    const char **command;
};

typedef struct Rule Rule;
struct Rule {
    char *class, *instance;
    bool isfloating;
};

// EWMH atoms
enum { NetSupported, NetCurrentDesktop, NetNumberOfDesktops, NetWMWindowType,
    NetWMWindowTypeDialog, NetWMWindowTypeMenu, NetWMWindowTypeSplash,
    NetWMWindowTypeToolbar, NetWMWindowTypeUtility, NetWMState,
    NetWMStateFullscreen, NetWMStateAbove, NetActiveWindow, NetLast };

// cursors
enum { CurNormal, CurResize, CurMove, CurLast };

static Atom net_atoms[NetLast];
static Cursor cursors[CurLast];

static Client *head, *focused;
static unsigned int total_clients, floating_clients;
static Display *dpy;
static XWindowAttributes attr;
static XButtonEvent prev_pointer_position;
static int screen;
static bool isrunning;
static bool fullscreen_lock;

static void quit(Arg arg);
static void die(char *exit_msg);
static void start();
static void stop();
static void grab();
static void loop();
static int ignore();
static void sigchld(int unused);
static void setup_ewmh_atoms();

// event handlers
static void keypress(XEvent *event);
static void buttonpress(XEvent *event);
static void buttonrelease(XEvent *event);
static void motionnotify(XEvent *event);
static void maprequest(XEvent *event);
static void destroynotify(XEvent *event);
static void configurerequest(XEvent *event);
static void enternotify(XEvent *event);

static void save_ws(unsigned int ws);
static void load_ws(unsigned int ws);
static void switch_ws(Arg arg);
static void ewmh_set_current_desktop(unsigned int ws);

// client operations
static void add_client(Window win);
static void remove_client(Window win);
static void tile();
static void tile_slaves(Client *pseudohead);
static void focus(Client *client);
static void focus_adjacent(Arg arg);
static void swap(Client *focused_client, Client *target_client);
static void zoom(Arg arg);
static void move_client(Arg arg);
static void kill_client(Arg arg);
static void toggle_fullscreen(Arg arg);
static void change_master_size(Arg arg);
static void apply_window_state(Client *client);
static void apply_rules(Client *client);
static void show_clients();
static void hide_clients();

static void send_to_ws(Arg arg);

static bool sendevent(Window win, Atom proto);
static Atom get_atom_prop(Window win, Atom atom);

typedef struct Key Key;
struct Key {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(const Arg arg);
    Arg arg;
};

#include "config.h"

struct {
    Client *head, *focused;
    unsigned int total_clients, floating_clients;
    bool fullscreen_lock;
} workspaces[MAX_WORKSPACES];

static unsigned int current_ws;

static void (*handle_events[LASTEvent])(XEvent *event) = {
    [KeyPress] = keypress,
    [ButtonPress] = buttonpress,
    [ButtonRelease] = buttonrelease,
    [MotionNotify] = motionnotify,
    [MapRequest] = maprequest,
    [ConfigureRequest] = configurerequest,
    [DestroyNotify] = destroynotify,
    [EnterNotify] = enternotify,
};

int main() {
    start();
    grab();
    loop();
    stop();
}

void die(char *exit_msg) {
    fprintf(stderr, "devoidwm: %s\n", exit_msg);
    exit(EXIT_FAILURE);
}

void start() {
    if (!(dpy = XOpenDisplay(0)))
        die("failed to open display");

    // install a sigchl handler
    sigchld(0);
    XSetErrorHandler(ignore);

    // the default screen
    screen = DefaultScreen(dpy);

    // root window
    root.win = DefaultRootWindow(dpy);
    root.width = XDisplayWidth(dpy, screen) - (margin_left + margin_right);
    root.height = XDisplayHeight(dpy, screen) - (margin_top + margin_bottom);

    // for quiting wm
    isrunning = true;

    // fullscreen lock
    fullscreen_lock = false;

    head = focused = NULL;
    total_clients = floating_clients = 0;

    // get MapRequest events
    XSelectInput(dpy, root.win, SubstructureRedirectMask);

    // set cursors
    cursors[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
    cursors[CurResize] = XCreateFontCursor(dpy, XC_sizing);
    cursors[CurMove] = XCreateFontCursor(dpy, XC_fleur);

    // define the cursor
	XDefineCursor(dpy, root.win, cursors[CurNormal]);

    // initialise workspaces
    for (unsigned int i = 0; i < MAX_WORKSPACES; i ++) {
        workspaces[i].focused = workspaces[i].head = NULL;
        workspaces[i].total_clients = workspaces[i].floating_clients = 0;
        workspaces[i].fullscreen_lock = false;
    }

    // set EWMH atoms
    setup_ewmh_atoms();
}

void setup_ewmh_atoms() {
    net_atoms[NetSupported] = GETATOMIDENTIFIER("_NET_SUPPORTED");
    net_atoms[NetNumberOfDesktops] = GETATOMIDENTIFIER("_NET_NUMBER_OF_DESKTOPS");
    net_atoms[NetCurrentDesktop] = GETATOMIDENTIFIER("_NET_CURRENT_DESKTOP");
    net_atoms[NetWMState] = GETATOMIDENTIFIER("_NET_WM_STATE");
    net_atoms[NetWMStateFullscreen] = GETATOMIDENTIFIER("_NET_WM_STATE_FULLSCREEN");
    net_atoms[NetWMStateAbove] = GETATOMIDENTIFIER("_NET_WM_STATE_ABOVE");
    net_atoms[NetWMWindowType] = GETATOMIDENTIFIER("_NET_WM_WINDOW_TYPE");
    net_atoms[NetWMWindowTypeDialog] = GETATOMIDENTIFIER("_NET_WM_WINDOW_TYPE_DIALOG");
    net_atoms[NetWMWindowTypeMenu] = GETATOMIDENTIFIER("_NET_WM_WINDOW_TYPE_MENU");
    net_atoms[NetWMWindowTypeSplash] = GETATOMIDENTIFIER("_NET_WM_WINDOW_TYPE_SPLASH");
    net_atoms[NetWMWindowTypeToolbar] = GETATOMIDENTIFIER("_NET_WM_WINDOW_TYPE_TOOLBAR");
    net_atoms[NetWMWindowTypeUtility] = GETATOMIDENTIFIER("_NET_WM_WINDOW_TYPE_UTILITY");
    net_atoms[NetActiveWindow] = GETATOMIDENTIFIER("_NET_ACTIVE_WINDOW");

    CHANGEATOMPROP(net_atoms[NetSupported], XA_ATOM, (unsigned char *)net_atoms ,NetLast);
    ewmh_set_current_desktop(0);

    unsigned long data[1];
    data[0] = MAX_WORKSPACES;
    CHANGEATOMPROP(net_atoms[NetNumberOfDesktops], XA_ATOM, (unsigned char *)data, 1);
}

void stop() {
    unsigned int n;
    Window root_return, parent_return, *children;

    // Kill every last one of them
    if (XQueryTree(dpy, root.win, &parent_return, &root_return, &children, &n))
        for (unsigned int i = 0; i < n; i ++)
            sendevent(children[i], XInternAtom(dpy, "WM_DELETE_WINDOW", True));

    XUngrabKey(dpy, AnyKey, AnyModifier, root.win);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root.win, net_atoms[NetActiveWindow]);
    XCloseDisplay(dpy);
}

int ignore() {
    return 0;
}

// Taken from dwm
void sigchld(int unused) {
    (void)unused;
    if(signal(SIGCHLD, sigchld) == SIG_ERR) die("Can't install SIGCHLD handler");
    while(0 < waitpid(-1, NULL, WNOHANG));
}

void grab() {
    for (size_t i = 0; i < sizeof(keys) / sizeof(Key); i ++)
        XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym), keys[i].modifier,
            root.win, True, GrabModeAsync, GrabModeAsync);

    XGrabButton(dpy, 1, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync,
                GrabModeAsync, None, None);
    XGrabButton(dpy, 3, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync,
                GrabModeAsync, None, None);
}

void loop() {
    XEvent ev;
    while (isrunning && !XNextEvent(dpy, &ev))
        if (handle_events[ev.type]) handle_events[ev.type](&ev);
}

void quit(Arg arg) {
    (void)arg;
    isrunning = false;
}

void keypress(XEvent *event) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(Key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym &&
            MODCLEAN(keys[i].modifier) == MODCLEAN(event -> xkey.state))
            keys[i].execute(keys[i].arg);
    }
}

void buttonpress(XEvent *event) {
    if(event -> xbutton.subwindow == None ||
        (event -> xbutton.button == 1 && event -> xbutton.button == 3)) return;

    XGrabPointer(dpy, event -> xbutton.subwindow, True,
        PointerMotionMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
        event -> xbutton.button == 1 ? cursors[CurMove] : cursors[CurResize],
        CurrentTime);

    XGetWindowAttributes(dpy, event -> xbutton.subwindow, &attr);
    prev_pointer_position = event -> xbutton;

    Client *client = head;
    for (unsigned int i = 0; i < total_clients; i ++, client = client -> next) {
        if (event -> xbutton.subwindow == client -> win) {
            if (!client -> isfloating) {
                client -> isfloating = true;
                floating_clients ++;
                tile();
            }
            break;
        }
    }
}

void motionnotify(XEvent *event) {
    while(XCheckTypedEvent(dpy, MotionNotify, event));
    int dx = event -> xbutton.x_root - prev_pointer_position.x_root;
    int dy = event -> xbutton.y_root - prev_pointer_position.y_root;
    bool isLeftClick = prev_pointer_position.button == 1;

    focused -> x = attr.x + (isLeftClick ? dx : 0);
    focused -> y = attr.y + (isLeftClick ? dy : 0);
    focused -> width = MAX(1, attr.width + (isLeftClick ? 0 : dx));
    focused -> height = MAX(1, attr.height + (isLeftClick ? 0 : dx));

    MOVERESIZE(event -> xmotion.window, focused -> x, focused -> y,
               focused -> width, focused -> height);
}

void buttonrelease(XEvent *event) {
    (void)event;
    XUngrabPointer(dpy, CurrentTime);
}

void maprequest(XEvent *event) {
    if (fullscreen_lock) {
        Arg arg = {0};
        toggle_fullscreen(arg);
    }

    XMapRequestEvent *ev = &event -> xmaprequest;

    // emit DestroyNotify and EnterNotify event
    XSelectInput(dpy, ev -> window, StructureNotifyMask|EnterWindowMask);

    // For pinentry-gtk (and maybe some other programs)
    Client *client = head;
    for(unsigned int i = 0; i < total_clients; i++, client = client -> next)
        if(ev -> window == client -> win) {
            XMapWindow(dpy, ev -> window);
            focus(client);
            return;
        }

    XMapWindow(dpy, ev -> window);
    add_client(ev -> window);
    focus(focused);

    if (focused -> isfullscreen) {
        Arg arg = {0};
        toggle_fullscreen(arg);
    } else if (focused -> isfloating) {
        XGetWindowAttributes(dpy, focused -> win, &attr);
        focused -> x = attr.x;
        focused -> y = attr.y;
        focused -> width = attr.width;
        focused -> height = attr.height;
    } else tile();

    XSync(dpy, True);
}

void destroynotify(XEvent *event) {
    if (fullscreen_lock) {
        Arg arg = {0};
        toggle_fullscreen(arg);
    }

    XDestroyWindowEvent *ev = &event -> xdestroywindow;
    remove_client(ev -> window);
    focus(focused);
    tile();
    XSync(dpy, True);
}

void enternotify(XEvent *event) {
    if (focused == NULL) return;
    XCrossingEvent *ev = &event -> xcrossing;
    Client *client = head;
    do {
        if (client -> win == ev -> window) {
            focus(client);
            break;
        }
        client = client -> next;
    } while (client != NULL && client != head);
}

void add_client(Window win) {
    Client *new_client;
    if (!(new_client = (Client *)malloc(sizeof(Client))))
        die("memory allocation failed");

    new_client -> win = win;

    // insert new_client in circular doubly linked list
    if (head == NULL) {
        head = new_client;
        head -> next = head -> prev = NULL;
    } else if (head -> next == NULL) {
        head -> next = head -> prev = new_client;
        new_client -> next = new_client -> prev = head;
    } else {
        head -> prev -> next = new_client;
        new_client -> prev = head -> prev;
        new_client -> next = head;
        head -> prev = new_client;
    }

    total_clients ++;
    focused = new_client;
    apply_window_state(focused);
    apply_rules(focused);
    if (focused -> isfloating) floating_clients ++;
}

void tile() {
    if (head == NULL) return;

    Client *pseudohead = head;
    while (pseudohead -> isfloating) {
        pseudohead = pseudohead -> next;
        if (pseudohead == NULL || pseudohead == head) return;
    }

    pseudohead -> x = margin_left;
    pseudohead -> y = margin_top;
    pseudohead -> height = root.height;
    pseudohead -> width = (total_clients - floating_clients == 1) ? root.width : (int)(root.width * master_size) - gap/2;
    MOVERESIZE(pseudohead -> win, pseudohead -> x, pseudohead -> y, pseudohead -> width, pseudohead -> height);

    tile_slaves(pseudohead);
}

// things you do for gaps
void tile_slaves(Client *pseudohead) {
    int slavecount = total_clients - floating_clients - 1;
    if (slavecount == 0) return;

    Client *client = pseudohead -> next;
    unsigned int height = (root.height - (slavecount - 1) * gap) / slavecount;

    for (int i = 0; i < slavecount; i ++, client = client -> next) {
        if (client -> isfloating) {
            i--;
            continue;
        };
        client -> x = (int)(root.width * master_size) + gap/2;
        client -> y = i == 0 ? margin_top : margin_top + i * (height + gap);
        client -> width = (int)(root.width * (1 - master_size)) - gap/2;
        client -> height = height;
        MOVERESIZE(client -> win, client -> x, client -> y, client -> width, client -> height);
    }
}

void focus(Client *client) {
    focused = client;
    if (focused == NULL) return;
    XSetInputFocus(dpy, focused -> win, RevertToParent, CurrentTime);
    if (focused -> isfloating) XRaiseWindow(dpy, focused -> win);
    CHANGEATOMPROP(net_atoms[NetActiveWindow], XA_WINDOW, (unsigned char *)&focused -> win, 1)
	sendevent(focused -> win, XInternAtom(dpy, "WM_TAKE_FOCUS", False));
}

void focus_adjacent(Arg arg) {
    if (focused == NULL || focused -> next == NULL) return;
    focus(arg.i == 1 ? focused -> next : focused -> prev);
}

void remove_client(Window win) {
    Client *client = head;
    for (unsigned int i = 0; i < total_clients; i ++, client = client -> next) {
        if (client -> win != win) continue;

        if (client == head) head = head -> next;

        if (client -> next == NULL) break;
        else if (client -> next -> next == client) {
            client -> next -> next = NULL;
            client -> next -> prev = NULL;
        } else {
            client -> next -> prev = client -> prev;
            client -> prev -> next = client -> next;
        }
        break;
    }

    if (client == NULL) return;
    if (client -> isfloating) floating_clients --;
    focused = client -> prev;
    free(client);
    total_clients --;
}

void swap(Client *focused_client, Client *target_client) {
    if (focused_client == NULL || target_client == NULL) return;

    Window temp = focused_client -> win;
    focused_client -> win = target_client -> win;
    target_client -> win = temp;

    MOVERESIZE(focused_client -> win, focused_client -> x, focused_client -> y,
               focused_client -> width, focused_client -> height);
    MOVERESIZE(target_client -> win, target_client -> x, target_client -> y,
               target_client -> width, target_client -> height);

    focus(target_client);
}

void zoom(Arg arg) {
    (void)arg;
    swap(focused, head);
}

void move_client(Arg arg) {
    (void)arg;
    swap(focused, arg.i == 1 ? focused -> next : focused -> prev);
}

void configurerequest(XEvent *event) {
    XConfigureRequestEvent *ev = &event -> xconfigurerequest;
    XWindowChanges wc;
    wc.x = ev -> x;
    wc.y = ev -> y;
    wc.width = ev -> width;
    wc.height = ev -> height;
    wc.border_width = ev -> border_width;
    wc.sibling = ev -> above;
    wc.stack_mode = ev -> detail;
    XConfigureWindow(dpy, ev -> window, ev -> value_mask, &wc);

    Client *client = head;
    for (unsigned int i = 0; i < total_clients; i ++, client = client -> next) {
        if (client -> win != ev -> window) continue;
        client -> x = ev -> x;
        client -> y = ev -> y;
        client -> width = ev -> width;
        client -> height = ev -> height;
    }
}

void kill_client(Arg arg) {
    (void)arg;
    if (focused == NULL) return;

    // send kill signal to window
    if (!sendevent(focused -> win, XInternAtom(dpy, "WM_DELETE_WINDOW", True))) {
        // If the client rejects it, we close it down the brutal way
        XGrabServer(dpy);
        XSetCloseDownMode(dpy, DestroyAll);
        XKillClient(dpy, focused -> win);
        XSync(dpy, False);
        XUngrabServer(dpy);
    }
}

void save_ws(unsigned int ws) {
    workspaces[ws].head = head;
    workspaces[ws].focused = focused;
    workspaces[ws].total_clients = total_clients;
    workspaces[ws].floating_clients = floating_clients;
    workspaces[ws].fullscreen_lock = fullscreen_lock;
}

void load_ws(unsigned int ws) {
    head = workspaces[ws].head;
    focused = workspaces[ws].focused;
    total_clients = workspaces[ws].total_clients;
    floating_clients = workspaces[ws].floating_clients;
    fullscreen_lock = workspaces[ws].fullscreen_lock;
}

void switch_ws(Arg arg) {
    if ((unsigned int)arg.i == current_ws) return;

    save_ws(current_ws);

    /* This works better than XMapWindow and XUnmapWindow
     * Also helps minimize screen flicker
     */

    // show new clients first
    load_ws(arg.i);
    show_clients();

    // hide old clients
    load_ws(current_ws);
    hide_clients();

    load_ws(arg.i);
    current_ws = arg.i;
    ewmh_set_current_desktop(current_ws);
    focus(focused);
    XSync(dpy, True);
}

void toggle_fullscreen(Arg arg) {
    (void)arg;
    if (focused == NULL || focused -> isfullscreen != fullscreen_lock) return;

    if (!focused -> isfullscreen) {
        focused -> x = 0;
        focused -> y = 0;
        focused -> width = XDisplayWidth(dpy, screen);
        focused -> height = XDisplayHeight(dpy, screen);
        MOVERESIZE(focused -> win, focused -> x, focused -> y, focused ->
                   width, focused -> height);
        CHANGEATOMPROP(net_atoms[NetWMState], XA_ATOM, (unsigned
                       char*)&net_atoms[NetWMStateFullscreen], 1);
        XRaiseWindow(dpy, focused -> win);
    } else tile();

    fullscreen_lock = !fullscreen_lock;
    focused -> isfullscreen = fullscreen_lock;

    XSync(dpy, True);
}

void ewmh_set_current_desktop(unsigned int ws) {
    unsigned long data[1];
    data[0] = ws + 1;
    CHANGEATOMPROP(net_atoms[NetCurrentDesktop], XA_CARDINAL,
                   (unsigned char *)data, 1);
}

void apply_window_state(Client *client) {
    client -> isfloating = false;
    client -> isfullscreen = false;

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
    if (prop == net_atoms[NetWMStateAbove]) client -> isfloating = true;
    else if (prop == net_atoms[NetWMStateFullscreen])
        client -> isfullscreen = true;
}

Atom get_atom_prop(Window win, Atom atom) {
    Atom prop = (unsigned long)NULL, da;
    unsigned char *prop_ret = NULL;
    int di;
    unsigned long dl;
    if (XGetWindowProperty(dpy, win, atom, 0, 1, False, XA_ATOM, &da, &di, &dl,
            &dl, &prop_ret) == Success) {
        if (prop_ret) {
            prop = ((Atom *)prop_ret)[0];
            XFree(prop_ret);
        }
    }
    return prop;
}

// Taken from dwm
bool sendevent(Window win, Atom proto) {
    int n;
    Atom *protocols;
    bool exists = false;
    XEvent ev;

    if (XGetWMProtocols(dpy, win, &protocols, &n)) {
        while (!exists && n--) exists = protocols[n] == proto;
        XFree(protocols);
    }

    if (exists) {
        ev.type = ClientMessage;
        ev.xclient.window = win;
        ev.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = proto;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, win, False, NoEventMask, &ev);
    }
    return exists;
}

void change_master_size(Arg arg) {
    float new_size = master_size + arg.i/100.0;
    if (new_size <= 0 || new_size >= 1) return;
    master_size = new_size;
    tile();
}

void apply_rules(Client *client) {
    XClassHint hints = {NULL, NULL};
    XGetClassHint(dpy, client -> win, &hints);
    for (size_t i = 0; i < sizeof(rules) / sizeof(Rule); i ++)
        if ((rules[i].class != NULL && strcmp(rules[i].class, hints.res_class) == 0) ||
            (rules[i].instance != NULL && strcmp(rules[i].instance, hints.res_name) == 0))
            client -> isfloating = rules[i].isfloating;
}

void send_to_ws(Arg arg) {
    if ((unsigned int)arg.i == current_ws || focused == NULL) return;

    Client *temp = focused;
    save_ws(current_ws);
    load_ws(arg.i);
    add_client(temp -> win);
    if (focused -> isfloating) {
        focused -> x = temp -> x;
        focused -> y = temp -> y;
        focused -> width = temp -> width;
        focused -> height = temp -> height;
    } else tile();

    hide_clients();

    save_ws(arg.i);
    load_ws(current_ws);
    remove_client(temp -> win);
    focus(focused);
    tile();
    XSync(dpy, True);
}

void show_clients() {
    Client *client = head;
    for (unsigned int i = 0; i < total_clients; i ++, client = client -> next)
        XMoveWindow(dpy, client -> win, client -> x, client -> y);
}

void hide_clients() {
    Client *client = head;
    for (unsigned int i = 0; i < total_clients; i ++, client = client -> next)
        XMoveWindow(dpy, client -> win, XDisplayWidth(dpy, screen), XDisplayHeight(dpy, screen));
}
