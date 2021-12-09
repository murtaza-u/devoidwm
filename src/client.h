#ifndef CLIENT_H
#define CLIENT_H

#define ISVISIBLE(c) (c -> tags & seltags)

#include <X11/Xlib.h>
#include <stdbool.h>

#include "key.h"

typedef struct Client Client;
struct Client {
    Window win;
    int x, y;
    unsigned int width, height;
    Client *next;
    bool isfloating, isfullscreen;
    unsigned int tags;
};

void attach(Client *client);
void detach(Client *client);
Client* wintoclient(Window win);
Client* nexttiled(Client *c);
Client* prevtiled(Client *c);
Client* nextvisible(Client *c);
Client* prevvisible(Client *c);
Client* newclient(Window win);
Client* get_visible_head();
Client* get_visible_tail();
void focus(Client *c);
void togglefullscreen(Arg arg);
void showhide(Client *c);
void killclient(Arg arg);
void swap(Client *focused_client, Client *target_client);
void zoom(Arg arg);
void resize(Client *client);

#endif
