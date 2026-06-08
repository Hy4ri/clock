#include "clock.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // Connect to X Server
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    // Initial window size
    int width = 650;
    int height = 320;

    // Create window
    unsigned long black = BlackPixel(dpy, screen);
    Window win = XCreateSimpleWindow(dpy, root, 0, 0, width, height, 1, black, black);
    XStoreName(dpy, win, "clock");

    // Select input masks: Exposures, Resizes (StructureNotify), and Keyboard events
    XSelectInput(dpy, win, ExposureMask | KeyPressMask | StructureNotifyMask);

    // Initialize clock rendering resources
    if (!clock_init(dpy, screen)) {
        fprintf(stderr, "clock_init failed\n");
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }

    // Map window to make it visible
    XMapWindow(dpy, win);

    // Draw initial frame and flush requests to ensure window is mapped and rendered immediately
    clock_draw(dpy, win, screen, width, height, true);
    XFlush(dpy);

    int x11_fd = ConnectionNumber(dpy);
    bool running = true;

    while (running) {
        fd_set in_fds;
        FD_ZERO(&in_fds);
        FD_SET(x11_fd, &in_fds);

        // Calculate remaining seconds until the next minute starts to align wakeups
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        int seconds_left = 60 - t->tm_sec;

        struct timeval tv;
        tv.tv_sec = seconds_left;
        tv.tv_usec = 0;

        select(x11_fd + 1, &in_fds, NULL, NULL, &tv);

        // Process all pending X11 events
        bool force_redraw = false;
        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            if (ev.type == ConfigureNotify) {
                width = ev.xconfigure.width;
                height = ev.xconfigure.height;
                force_redraw = true;
            } else if (ev.type == Expose) {
                force_redraw = true;
            } else if (ev.type == KeyPress) {
                KeySym keysym = XLookupKeysym(&ev.xkey, 0);
                if (keysym == XK_q || keysym == XK_Q || keysym == XK_Escape) {
                    running = false;
                } else if (keysym == XK_equal || keysym == XK_plus || keysym == XK_KP_Add) {
                    clock_zoom_in();
                    force_redraw = true;
                } else if (keysym == XK_minus || keysym == XK_KP_Subtract) {
                    clock_zoom_out();
                    force_redraw = true;
                } else if (keysym == XK_r || keysym == XK_0) {
                    clock_zoom_reset();
                    force_redraw = true;
                }
            }
        }

        // Draw clock (double-buffered)
        if (clock_draw(dpy, win, screen, width, height, force_redraw)) {
            XFlush(dpy);
        }
    }

    // Clean up
    clock_cleanup(dpy, screen);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    return 0;
}
