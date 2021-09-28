#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>

#define MAX_WORKSPACES 9

#define MODCLEAN(mask) (mask & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

#define MOVERESIZE(win, x, y, width, height) \
    XMoveResizeWindow(dpy, win, x, y, width, height)

typedef struct Client Client;
struct Client {
    int x, y, old_x, old_y;
    unsigned int width, height, old_width, old_height;
    Client *next, *prev;
    Window win;
};

static Client *head, *focused;
static unsigned int total_clients;
static Display *dpy;
static int screen;
static bool isrunning;
static unsigned int margin_top = 26;
static unsigned int margin_right = 6;
static unsigned int margin_bottom = 6;
static unsigned int margin_left = 6;
static unsigned int gap = 20;

struct {
    Window win;
    unsigned int width, height;
} root;

static void die(char *exit_msg);
static void start();
static void stop();
static void grab();
static void loop();
static int ignore();
static void sigchld(int unused);

// event handlers
static void keypress(XEvent *event);
static void maprequest(XEvent *event);
static void destroynotify(XEvent *event);
static void configurerequest(XEvent *event);

typedef union Arg Arg;
union Arg {
    const int i;
    const char **command;
};

typedef struct Key Key;
struct Key {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(const Arg arg);
    Arg arg;
};

struct {
    Client *head, *focused;
    unsigned int total_clients;
} workspaces[MAX_WORKSPACES];

static unsigned int current_ws;

static void save_ws();
static void load_ws();
static void switch_ws(Arg arg);

// client operations
static void add_client(Window win);
static void remove_client(Window win);
static void tile();
static void tile_master();
static void tile_slaves();
static void focus(Window win);
static void focus_adjacent(Arg arg);
static void swap(Client *focused_client, Client *target_client);
static void zoom(Arg arg);
static void move_client(Arg arg);
static void kill_client(Arg arg);

static void quit(Arg arg);

static const unsigned int MODKEY = Mod4Mask;
static const Key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, {0}},
    {MODKEY, XK_j, focus_adjacent, {.i = 1}},
    {MODKEY, XK_k, focus_adjacent, {.i = -1}},
    {MODKEY, XK_space, zoom, {0}},
    {MODKEY|ShiftMask, XK_j, move_client, {.i = 1}},
    {MODKEY|ShiftMask, XK_k, move_client, {.i = -1}},
    {MODKEY, XK_x, kill_client, {0}},
    {MODKEY, XK_1, switch_ws, {.i = 0}},
    {MODKEY, XK_2, switch_ws, {.i = 1}},
    {MODKEY, XK_3, switch_ws, {.i = 2}},
    {MODKEY, XK_4, switch_ws, {.i = 3}},
    {MODKEY, XK_5, switch_ws, {.i = 4}},
    {MODKEY, XK_6, switch_ws, {.i = 5}},
    {MODKEY, XK_7, switch_ws, {.i = 6}},
    {MODKEY, XK_8, switch_ws, {.i = 7}},
    {MODKEY, XK_9, switch_ws, {.i = 8}},
};

static void (*handle_events[LASTEvent])(XEvent *event) = {
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [ConfigureRequest] = configurerequest,
    [DestroyNotify] = destroynotify,
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

    head = focused = NULL;
    total_clients = 0;

    // get MapRequest events
    XSelectInput(dpy, root.win, SubstructureRedirectMask);

    // define the cursor
	XDefineCursor(dpy, root.win, XCreateFontCursor(dpy, 68));

    // initialise workspaces
    for (unsigned int i = 0; i < MAX_WORKSPACES; i ++) {
        workspaces[i].focused = workspaces[i].head = NULL;
        workspaces[i].total_clients = 0;
    }
}

void stop() {
    XUngrabKey(dpy, AnyKey, AnyModifier, root.win);
    XSync(dpy, False);
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
    for (size_t i = 0; i < sizeof(keys) / sizeof(Key); i ++) {
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
        if (keysym == keys[i].keysym
                && MODCLEAN(keys[i].modifier) == MODCLEAN(event -> xkey.state))
            keys[i].execute(keys[i].arg);
    }
}

void maprequest(XEvent *event) {
    XMapRequestEvent *ev = &event -> xmaprequest;

    // emit event a destroynotify event on kill
    XSelectInput(dpy, ev -> window, StructureNotifyMask);

    // For pinentry-gtk (and maybe some other programs)
    Client *client = head;
    for(unsigned int i = 0; i < total_clients; i++, client = client -> next)
        if(ev -> window == client -> win) {
            XMapWindow(dpy,ev -> window);
            return;
        }

    XMapWindow(dpy, ev -> window);
    focus(ev -> window);
    add_client(ev -> window);
    tile();
}

void destroynotify(XEvent *event) {
    XDestroyWindowEvent *ev = &event -> xdestroywindow;
    remove_client(ev -> window);
    tile();
    if (focused != NULL) focus(focused -> win);
}

void add_client(Window win) {
    Client *new_client;
    if (!(new_client = (Client *)malloc(sizeof(Client))))
        die("memory allocation failed");

    new_client -> win = win;

    // insert new_client in circualar doubly linked list
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

    focused = new_client;
    total_clients ++;
}

void tile() {
    tile_master();
    tile_slaves();
}

void tile_master() {
    if (head == NULL) return;
    head -> x = margin_left;
    head -> y = margin_top;
    head -> height = root.height;
    head -> width = (head -> next == NULL) ? root.width : root.width/2 - gap/2;
    XMoveResizeWindow(dpy, head -> win, head -> x, head -> y, head -> width, head -> height);
}

// things you do for gaps
void get_slave_props(Client *slave, unsigned int i) {
    unsigned int slavecount = total_clients - 1;
    unsigned int usedheight = 0;
    slave -> x = root.width/2 + gap/2;
    slave -> y = i == 0 ? margin_top : (i * root.height)/slavecount + gap;
    slave -> width = root.width/2 - gap/2;
    slave -> height = slavecount == 1 ? root.height : root.height/slavecount - gap/2;

    usedheight += slave -> height;

    // fill gap caused due to integer division
    if (slavecount == i) slave -> height += root.height - usedheight;
}

void tile_slaves() {
    if (head == NULL || head -> next == NULL) return;
    Client *client = head -> next;
    for (unsigned int i = 0; i < total_clients - 1; i ++, client = client -> next) {
        get_slave_props(client, i);
        XMoveResizeWindow(dpy, client -> win, client -> x, client -> y, client -> width, client -> height);
    }
}

void focus(Window win) {
    XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, win);
}

void focus_adjacent(Arg arg) {
    if (focused == NULL || focused -> next == NULL) return;
    focused = arg.i == 1 ? focused -> next : focused -> prev;
    focus(focused -> win);
}

void remove_client(Window win) {
    Client *client = head;
    for (unsigned int i = 0; i < total_clients && client != NULL; i ++, client = client -> next) {
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
    focused = client -> prev;
    free(client);
    total_clients --;
}

void swap(Client *focused_client, Client *target_client) {
    if (focused_client == NULL || target_client == NULL) return;

    Window temp = focused_client -> win;
    focused_client -> win = target_client -> win;
    target_client -> win = temp;

    MOVERESIZE(focused_client -> win, focused_client -> x, focused_client -> y, focused_client -> width, focused_client -> height);
    MOVERESIZE(target_client -> win, target_client -> x, target_client -> y, target_client -> width, target_client -> height);

    focus(target_client -> win);
    focused = target_client;
}

void zoom(Arg arg) {
    (void)arg;
    swap(focused, head);
}

void move_client(Arg arg) {
    (void)arg;
    swap(focused, arg.i == 1 ? focused -> next : focused -> prev);
}

// Taken from dwm
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
}

// Taken from catwm
void kill_client(Arg arg) {
    (void)arg;
    if(focused != NULL) {
        // send kill signal to window
        XEvent kill_event;
        kill_event.type = ClientMessage;
        kill_event.xclient.window = focused->win;
        kill_event.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
        kill_event.xclient.format = 32;
        kill_event.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
        kill_event.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, focused->win, False, NoEventMask, &kill_event);
    }
}

void save_ws() {
    workspaces[current_ws].head = head;
    workspaces[current_ws].focused = focused;
    workspaces[current_ws].total_clients = total_clients;
}

void load_ws() {
    head = workspaces[current_ws].head;
    focused = workspaces[current_ws].focused;
    total_clients = workspaces[current_ws].total_clients;
}

void switch_ws(Arg arg) {
    if ((unsigned int)arg.i == current_ws) return;
    save_ws();

    Client *client = head;

    // This works better than XMapWindow and XUnmapWindow
    for (unsigned int i = 0; i < total_clients; i ++, client = client -> next) {
        client -> old_x = client -> x;
        client -> old_y = client -> y;
        client -> old_width = client -> width;
        client -> old_height = client -> height;

        // Move clients off the screen where they are invisible
        client -> x = root.width;
        client -> y = root.height;

        // Resize window to min value. Minimizes screen flicker
        client -> width = 1;
        client -> height = 1;

        MOVERESIZE(client -> win, client -> x, client -> y, client -> width, client -> height);
    }

    current_ws = arg.i;
    load_ws();

    if (focused != NULL) focus(focused -> win);

    client = head;
    for (unsigned int i = 0; i < total_clients; i ++, client = client -> next) {
        client -> x = client -> old_x;
        client -> y = client -> old_y;
        client -> width = client -> old_width;
        client -> height = client -> old_height;
        MOVERESIZE(client -> win, client -> x, client -> y, client -> width, client -> height);
    }
}
