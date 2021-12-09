#ifndef DEVOID_H
#define DEVOID_H

#include <stdbool.h>

#include "client.h"

#define MAX(a, b) (a) > (b) ? (a) : (b)

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
extern Client *head, *sel;
extern bool fullscreenlock;
extern unsigned int seltags;

struct Root {
    Window win;
    int x, y;
    unsigned int width, height;
};
extern struct Root root;

#endif