#ifndef CONFIG_H
#define CONFIG_H

#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include "src/key.h"
#include "src/client.h"
#include "src/tags.h"
#include "src/focus.h"

/* maximum no. of workspaces */
#define MAX_WORKSPACES 9

/* size of the master window range -> (0, 1) */
static float mratio = 0.5;

/* no. of windows in the master area */
static unsigned int nmaster = 1;

/* margin */
static unsigned int margin_top = 6;
static unsigned int margin_right = 6;
static unsigned int margin_bottom = 6;
static unsigned int margin_left = 6;

/* gap between 2 windows */
static unsigned int gap = 10;

/* Border around windows */
static char focused_border_color[] = "#ffffff";
static char normal_border_color[] = "#10151a";
static unsigned int border_width = 1;

// static const Rule rules[] = {
//     /* xprop:
//      * WM_CLASS(STRING) = class
//      * WM_NAME(STRING) = title
//      */
//
//     /* class            title       isfloating      isfullscreen */
//     {"Gcolor3",         NULL,       1,              0            },
//     {"mpv",             NULL,       0,              1            },
// };

/* Mod4Mask -> super key
 * Mod1Mask -> Alt key
 * ControlMask -> control key
 * ShiftMask -> shift key
 */

/* modifier key */
static const unsigned int MODKEY = Mod4Mask;

static const Key keys[] = {
    /* quit devoidwm */
    {MODKEY|ShiftMask, XK_q, quit, {0}},

    /* focus the next/prev window */
    {MODKEY, XK_j, focus_adjacent, {.i = 1}},
    {MODKEY, XK_k, focus_adjacent, {.i = -1}},

    /* swap slave window with the master window */
    {MODKEY, XK_space, zoom, {0}},

    /* rotate a window through the stack */
    // {MODKEY|ShiftMask, XK_j, move_client, {.i = 1}},
    // {MODKEY|ShiftMask, XK_k, move_client, {.i = -1}},

    /* kill a window */
    {MODKEY, XK_x, killclient, {0}},

    /* toggle fullscreen */
    {MODKEY|ShiftMask, XK_f, togglefullscreen, {0}},

    /* change the size of the master window */
    // {MODKEY, XK_h, change_master_size, {.i = -5}},
    // {MODKEY, XK_l, change_master_size, {.i = 5}},

    /* increment/decrement no. of windows in master area */
    // {MODKEY, XK_i, incmaster, {.i = 1}},
    // {MODKEY, XK_d, incmaster, {.i = -1}},

    /* switch workspaces */
    {MODKEY, XK_1, view, {.ui = 1 << 0}},
    {MODKEY, XK_2, view, {.ui = 1 << 1}},
    {MODKEY, XK_3, view, {.ui = 1 << 2}},
    {MODKEY, XK_4, view, {.ui = 1 << 3}},
    {MODKEY, XK_5, view, {.ui = 1 << 4}},
    {MODKEY, XK_6, view, {.ui = 1 << 5}},
    {MODKEY, XK_7, view, {.ui = 1 << 6}},
    {MODKEY, XK_8, view, {.ui = 1 << 7}},
    {MODKEY, XK_9, view, {.ui = 1 << 8}},

    {MODKEY|ControlMask, XK_1, toggletag, {.ui = 1 << 0}},
    {MODKEY|ControlMask, XK_2, toggletag, {.ui = 1 << 1}},
    {MODKEY|ControlMask, XK_3, toggletag, {.ui = 1 << 2}},
    {MODKEY|ControlMask, XK_4, toggletag, {.ui = 1 << 3}},
    {MODKEY|ControlMask, XK_5, toggletag, {.ui = 1 << 4}},
    {MODKEY|ControlMask, XK_6, toggletag, {.ui = 1 << 5}},
    {MODKEY|ControlMask, XK_7, toggletag, {.ui = 1 << 6}},
    {MODKEY|ControlMask, XK_8, toggletag, {.ui = 1 << 7}},
    {MODKEY|ControlMask, XK_9, toggletag, {.ui = 1 << 8}},

    /* send focused client to a different workspace */
    // {MODKEY|ShiftMask, XK_1, send_to_ws, {.i = 0}},
    // {MODKEY|ShiftMask, XK_2, send_to_ws, {.i = 1}},
    // {MODKEY|ShiftMask, XK_3, send_to_ws, {.i = 2}},
    // {MODKEY|ShiftMask, XK_4, send_to_ws, {.i = 3}},
    // {MODKEY|ShiftMask, XK_5, send_to_ws, {.i = 4}},
    // {MODKEY|ShiftMask, XK_6, send_to_ws, {.i = 5}},
    // {MODKEY|ShiftMask, XK_7, send_to_ws, {.i = 6}},
    // {MODKEY|ShiftMask, XK_8, send_to_ws, {.i = 7}},
    // {MODKEY|ShiftMask, XK_9, send_to_ws, {.i = 8}},
};

#endif
