#ifndef ATOMS_H
#define ATOMS_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "client.h"

#define CHANGEATOMPROP(prop, type, data, nelments) \
    XChangeProperty(dpy, root.win, prop, type, 32, PropModeReplace, data, nelments);

#define GETATOMID(name) XInternAtom(dpy, name, False)

/* EWMH atoms */
enum { NetSupported, NetCurrentDesktop, NetNumberOfDesktops, NetWMWindowType,
    NetWMWindowTypeDialog, NetWMWindowTypeMenu, NetWMWindowTypeSplash,
    NetWMWindowTypeToolbar, NetWMWindowTypeUtility, NetWMState,
    NetWMStateFullscreen, NetWMStateAbove, NetActiveWindow, NetLast };

extern Atom net_atoms[NetLast];

Atom get_atom_prop(Window win, Atom atom);
void setup_ewmh_atoms();
void apply_window_state(Client *c);

#endif
