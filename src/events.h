#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

/* event handlers */
void keypress(XEvent *event);
void buttonpress(XEvent *event);
void buttonrelease(XEvent *event);
void motionnotify(XEvent *event);
void maprequest(XEvent *event);
void destroynotify(XEvent *event);
void enternotify(XEvent *event);
void clientmessage(XEvent *event);

void handle_event(XEvent *event);
bool sendevent(Window win, Atom proto);

#endif
