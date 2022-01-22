#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

bool sendevent(Window win, Atom proto);

/* event handlers */
void keypress(XEvent *event);
void buttonpress(XEvent *event);
void buttonrelease(XEvent *event);
void motionnotify(XEvent *event);
void maprequest(XEvent *event);
void destroynotify(XEvent *event);
void enternotify(XEvent *event);
void clientmessage(XEvent *event);
void unmapnotify(XEvent *event);
void configurerequest(XEvent *event);

extern void (*handle_events[LASTEvent])(XEvent *event);

#endif
