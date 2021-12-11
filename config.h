#ifndef CONFIG_H
#define CONFIG_H

#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include "src/key.h"
#include "src/client.h"
#include "src/tags.h"
#include "src/focus.h"

/* size of the master window. Range -> [0.05, 0.95] */
extern float mratio;

/* no. of windows in the master area */
extern unsigned int nmaster;

/* margin */
extern unsigned int margin_top;
extern unsigned int margin_right;
extern unsigned int margin_bottom;
extern unsigned int margin_left;

/* gap between 2 windows */
extern unsigned int gap;

/* Border around windows */
static const char focused_border_color[] = "#7ea89e";
static const char normal_border_color[] = "#10151a";
static const unsigned int border_width = 2;

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

    /* kill a window */
    {MODKEY, XK_x, killclient, {0}},

    /* toggle fullscreen */
    {MODKEY|ShiftMask, XK_f, togglefullscr, {0}},

    /* change the size of the master window */
    {MODKEY, XK_h, setmratio, {.f = -0.05}},
    {MODKEY, XK_l, setmratio, {.f = 0.05}},

    /* increment/decrement no. of windows in master area */
    {MODKEY, XK_i, incmaster, {.i = 1}},
    {MODKEY, XK_d, incmaster, {.i = -1}},

    {MODKEY, XK_0, view, {.ui = (1 << 9) - 1}},
    {MODKEY, XK_1, view, {.ui = 1 << 0}},
    {MODKEY, XK_2, view, {.ui = 1 << 1}},
    {MODKEY, XK_3, view, {.ui = 1 << 2}},
    {MODKEY, XK_4, view, {.ui = 1 << 3}},
    {MODKEY, XK_5, view, {.ui = 1 << 4}},
    {MODKEY, XK_6, view, {.ui = 1 << 5}},
    {MODKEY, XK_7, view, {.ui = 1 << 6}},
    {MODKEY, XK_8, view, {.ui = 1 << 7}},
    {MODKEY, XK_9, view, {.ui = 1 << 8}},

    {MODKEY|ControlMask, XK_0, toggletag, {.ui = (1 << 9) - 1}},
    {MODKEY|ControlMask, XK_1, toggletag, {.ui = 1 << 0}},
    {MODKEY|ControlMask, XK_2, toggletag, {.ui = 1 << 1}},
    {MODKEY|ControlMask, XK_3, toggletag, {.ui = 1 << 2}},
    {MODKEY|ControlMask, XK_4, toggletag, {.ui = 1 << 3}},
    {MODKEY|ControlMask, XK_5, toggletag, {.ui = 1 << 4}},
    {MODKEY|ControlMask, XK_6, toggletag, {.ui = 1 << 5}},
    {MODKEY|ControlMask, XK_7, toggletag, {.ui = 1 << 6}},
    {MODKEY|ControlMask, XK_8, toggletag, {.ui = 1 << 7}},
    {MODKEY|ControlMask, XK_9, toggletag, {.ui = 1 << 8}},

    {MODKEY|ShiftMask, XK_0, tag, {.ui = (1 << 9) - 1}},
    {MODKEY|ShiftMask, XK_1, tag, {.ui = 1 << 0}},
    {MODKEY|ShiftMask, XK_2, tag, {.ui = 1 << 1}},
    {MODKEY|ShiftMask, XK_3, tag, {.ui = 1 << 2}},
    {MODKEY|ShiftMask, XK_4, tag, {.ui = 1 << 3}},
    {MODKEY|ShiftMask, XK_5, tag, {.ui = 1 << 4}},
    {MODKEY|ShiftMask, XK_6, tag, {.ui = 1 << 5}},
    {MODKEY|ShiftMask, XK_7, tag, {.ui = 1 << 6}},
    {MODKEY|ShiftMask, XK_8, tag, {.ui = 1 << 7}},
    {MODKEY|ShiftMask, XK_9, tag, {.ui = 1 << 8}},
};

#endif
