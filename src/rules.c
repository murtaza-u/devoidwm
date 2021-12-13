#include <X11/Xutil.h>
#include <string.h>

#include "rules.h"
#include "devoid.h"
#include "../config.h"

void apply_rules(Client *client) {
    XClassHint hints = {NULL, NULL};
    XGetClassHint(dpy, client -> win, &hints);
    if (hints.res_class == NULL) hints.res_class = "";
    if (hints.res_name == NULL) hints.res_name = "";

    for (size_t i = 0; i < sizeof(rules) / sizeof(Rule); i ++) {
        if ((rules[i].classname != NULL && strcmp(rules[i].classname, hints.res_class) == 0) ||
            (rules[i].instance != NULL && strcmp(rules[i].instance, hints.res_name) == 0)) {
            client -> isfloating = rules[i].isfloating;
            client -> isfullscr = rules[i].isfullscreen;
        }
    }
}
