// maximum no. of workspaces
#define MAX_WORKSPACES 9

static float master_size = 0.6;
static unsigned int margin_top = 26;
static unsigned int margin_right = 6;
static unsigned int margin_bottom = 6;
static unsigned int margin_left = 6;
static unsigned int gap = 20; // gap between 2 windows

/* Mod4Mask -> super key
 * Mod1Mask -> Alt key
 * ControlMask -> control key
 * ShiftMask -> shift key
 */

// modifier key
static const unsigned int MODKEY = Mod4Mask;

static const Key keys[] = {
    // quit devoidwm
    {MODKEY|ShiftMask, XK_q, quit, {0}},

    // focus the next/prev window
    {MODKEY, XK_j, focus_adjacent, {.i = 1}},
    {MODKEY, XK_k, focus_adjacent, {.i = -1}},

    // swap slave window with the master window
    {MODKEY, XK_space, zoom, {0}},

    // rotate a window through the stack
    {MODKEY|ShiftMask, XK_j, move_client, {.i = 1}},
    {MODKEY|ShiftMask, XK_k, move_client, {.i = -1}},

    // kill a window
    {MODKEY, XK_x, kill_client, {0}},

    // switch workspaces
    {MODKEY, XK_1, switch_ws, {.i = 0}},
    {MODKEY, XK_2, switch_ws, {.i = 1}},
    {MODKEY, XK_3, switch_ws, {.i = 2}},
    {MODKEY, XK_4, switch_ws, {.i = 3}},
    {MODKEY, XK_5, switch_ws, {.i = 4}},
    {MODKEY, XK_6, switch_ws, {.i = 5}},
    {MODKEY, XK_7, switch_ws, {.i = 6}},
    {MODKEY, XK_8, switch_ws, {.i = 7}},
    {MODKEY, XK_9, switch_ws, {.i = 8}},

    // toggle fullscreen
    {MODKEY|ShiftMask, XK_f, toggle_fullscreen, {0}},
};
