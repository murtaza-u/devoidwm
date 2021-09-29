#define MAX_WORKSPACES 9

/* Mod4Mask -> super key
 * Mod1Mask -> Alt key
 */

static const unsigned int MODKEY = Mod4Mask;

static const Key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, {0}},
    {MODKEY, XK_j, focus_adjacent, {.i = 1}},
    {MODKEY, XK_k, focus_adjacent, {.i = -1}},
    {MODKEY, XK_space, zoom, {0}},
    {MODKEY|ShiftMask, XK_j, move_client, {.i = 1}},
    {MODKEY|ShiftMask, XK_k, move_client, {.i = -1}},
    {MODKEY, XK_x, kill_client, {0}},
    {MODKEY, XK_1, switch_ws, {.i = 0}},
    {MODKEY, XK_2, switch_ws, {.i = 1}},
    {MODKEY, XK_3, switch_ws, {.i = 2}},
    {MODKEY, XK_4, switch_ws, {.i = 3}},
    {MODKEY, XK_5, switch_ws, {.i = 4}},
    {MODKEY, XK_6, switch_ws, {.i = 5}},
    {MODKEY, XK_7, switch_ws, {.i = 6}},
    {MODKEY, XK_8, switch_ws, {.i = 7}},
    {MODKEY, XK_9, switch_ws, {.i = 8}},
    {MODKEY|ShiftMask, XK_f, toggle_fullscreen, {0}},
};
