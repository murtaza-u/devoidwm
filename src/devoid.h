#ifndef DEVOID_H
#define DEVOID_H

#include "client.h"
#include <stdbool.h>

void die(char *);
void sigchld(int);
int ignore();

void start();
void grab();
void loop();
void stop();

extern bool isrunning;
extern Display *dpy;
extern int screen;
extern XWindowAttributes attr;

struct Root {
    Window win;
    int x, y;
    unsigned int width, height;
    Client *head, *focused;
};
extern struct Root root;

#endif
