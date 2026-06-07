#ifndef CLOCK_H
#define CLOCK_H

#include <X11/Xlib.h>
#include <stdbool.h>

/*
 * Initialize clock resources (Xft colors and font parameters).
 */
bool clock_init(Display *dpy, int screen);

/*
 * Clean up Xft colors and fonts.
 */
void clock_cleanup(Display *dpy, int screen);

/*
 * Render the clock double-buffered using a Pixmap.
 */
void clock_draw(Display *dpy, Window win, int screen, int width, int height);

/*
 * Zoom controls
 */
void clock_zoom_in(void);
void clock_zoom_out(void);
void clock_zoom_reset(void);

#endif /* CLOCK_H */
