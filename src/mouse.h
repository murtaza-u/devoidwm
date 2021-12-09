#ifndef MOUSE_H
#define MOUSE_H

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

/* cursors */
enum { CurNormal, CurResize, CurMove, CurLast };

extern XButtonEvent prev_pointer_position;
extern Cursor cursors[CurLast];

void setup_cursor();
void handle_buttonpress(XEvent *event);
void handle_motionnotify(XEvent *event);

#endif
