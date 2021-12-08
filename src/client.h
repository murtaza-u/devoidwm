#ifndef CLIENT_H
#define CLIENT_H

#include <X11/Xlib.h>
#include <stdbool.h>

typedef struct Client Client;
struct Client {
    Window win;
    int x, y;
    unsigned int width, height;
    Client *next, *prev;
    bool isfloating, isfullscreen;
};

void attach(Client *client);
void detach(Client *client);
Client* wintoclient(Window win);
void focus(Client *c);

#endif
