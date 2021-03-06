#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ewmh.h"
#include "devoid.h"

Atom net_atoms[NetLast];

Atom get_atom_prop(Window win, Atom atom) {
    Atom prop = (unsigned long)NULL, da;
    unsigned char *prop_ret = NULL;
    int di;
    unsigned long dl;
    if (XGetWindowProperty(dpy, win, atom, 0, 1, False, XA_ATOM, &da, &di, &dl, &dl, &prop_ret)
        == Success) {
        if (prop_ret) {
            prop = ((Atom *)prop_ret)[0];
            XFree(prop_ret);
        }
    }
    return prop;
}

void setup_ewmh_atoms() {
    net_atoms[NetSupported] = GETATOMID("_NET_SUPPORTED");
    net_atoms[NetNumberOfDesktops] = GETATOMID("_NET_NUMBER_OF_DESKTOPS");
    net_atoms[NetCurrentDesktop] = GETATOMID("_NET_CURRENT_DESKTOP");
    net_atoms[NetWMState] = GETATOMID("_NET_WM_STATE");
    net_atoms[NetWMStateFullscreen] = GETATOMID("_NET_WM_STATE_FULLSCREEN");
    net_atoms[NetWMStateAbove] = GETATOMID("_NET_WM_STATE_ABOVE");
    net_atoms[NetWMWindowType] = GETATOMID("_NET_WM_WINDOW_TYPE");
    net_atoms[NetWMWindowTypeDialog] = GETATOMID("_NET_WM_WINDOW_TYPE_DIALOG");
    net_atoms[NetWMWindowTypeMenu] = GETATOMID("_NET_WM_WINDOW_TYPE_MENU");
    net_atoms[NetWMWindowTypeSplash] = GETATOMID("_NET_WM_WINDOW_TYPE_SPLASH");
    net_atoms[NetWMWindowTypeToolbar] = GETATOMID("_NET_WM_WINDOW_TYPE_TOOLBAR");
    net_atoms[NetWMWindowTypeUtility] = GETATOMID("_NET_WM_WINDOW_TYPE_UTILITY");
    net_atoms[NetActiveWindow] = GETATOMID("_NET_ACTIVE_WINDOW");

    CHANGEATOMPROP(net_atoms[NetSupported], XA_ATOM,
                (unsigned char *)net_atoms, NetLast);
}

void apply_window_state(Client *c) {
    c -> isfloating = 0;
    c -> isfullscr = 0;

    Atom prop = get_atom_prop(c -> win, net_atoms[NetWMWindowType]);
    if (prop == net_atoms[NetWMWindowTypeDialog] ||
            prop == net_atoms[NetWMWindowTypeMenu] ||
            prop == net_atoms[NetWMWindowTypeSplash] ||
            prop == net_atoms[NetWMWindowTypeToolbar] ||
            prop == net_atoms[NetWMWindowTypeUtility]) {
        c -> isfloating = 1;
        return;
    }

    prop = get_atom_prop(c -> win, net_atoms[NetWMState]);
    if (prop == net_atoms[NetWMStateAbove]) c -> isfloating = 1;
    else if (prop == net_atoms[NetWMStateFullscreen]) c -> isfullscr = 1;
}
