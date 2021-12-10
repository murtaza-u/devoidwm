#ifndef CLIENT_H
#define CLIENT_H

#include <X11/Xlib.h>
#include <stdbool.h>

#include "key.h"

typedef struct Client Client;
struct Client {
    Window win;
    int x, y;
    unsigned int width, height;
    Client *next;
    bool isfloating, isfullscr;
    unsigned int tags;
};

void attach(Client *client);
void detach(Client *client);
Client* wintoclient(Window win);

Client* nexttiled(Client *c, unsigned int tags);
Client* prevtiled(Client *c, unsigned int tags);
Client* nextvisible(Client *c, unsigned int tags);
Client* prevvisible(Client *c, unsigned int tags);

Client* newclient(Window win);
Client* get_visible_head();
Client* get_visible_tail();
void focus(Client *c);
void togglefullscr(Arg arg);
void showhide(Client *c);
void killclient(Arg arg);
void swap(Client *focused_client, Client *target_client);
void zoom(Arg arg);
void resize(Client *client);
void incmaster(Arg arg);
void setmratio(Arg arg);
unsigned int isvisible(Client *c, unsigned int tags);
void lock_fullscr(Client *c);
void unlock_fullscr(Client *c);

#endif
