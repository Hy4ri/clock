#include "clock.h"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static float zoomScale = 1.0f;
static bool zoomChanged = false;

// Cached time and zoom values to track changes
static int cached_min = -1;
static int cached_hour = -1;
static int cached_mday = -1;
static float cached_zoom = -1.0f;

// Color assets
static XftColor bgColor;
static XftColor fontColor;
static XftColor redColor;

// Cached font assets
static XftFont *cached_time_font = NULL;
static int cached_time_font_size = -1;

static XftFont *cached_date_font = NULL;
static int cached_date_font_size = -1;

// Cached graphics assets
static Pixmap cached_pixmap = None;
static XftDraw *cached_draw = NULL;
static int cached_pixmap_width = -1;
static int cached_pixmap_height = -1;

static XftFont *get_time_font(Display *dpy, int screen, int size) {
    if (cached_time_font && cached_time_font_size == size) {
        return cached_time_font;
    }
    if (cached_time_font) {
        XftFontClose(dpy, cached_time_font);
    }
    
    char font_query[512];
    sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", size);
    cached_time_font = XftFontOpenName(dpy, screen, font_query);
    if (!cached_time_font) {
        sprintf(font_query, "monospace:style=Bold:pixelsize=%d", size);
        cached_time_font = XftFontOpenName(dpy, screen, font_query);
    }
    cached_time_font_size = size;
    return cached_time_font;
}

static XftFont *get_date_font(Display *dpy, int screen, int size) {
    if (cached_date_font && cached_date_font_size == size) {
        return cached_date_font;
    }
    if (cached_date_font) {
        XftFontClose(dpy, cached_date_font);
    }
    
    char font_query[512];
    sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", size);
    cached_date_font = XftFontOpenName(dpy, screen, font_query);
    if (!cached_date_font) {
        sprintf(font_query, "monospace:style=Bold:pixelsize=%d", size);
        cached_date_font = XftFontOpenName(dpy, screen, font_query);
    }
    cached_date_font_size = size;
    return cached_date_font;
}

bool clock_init(Display *dpy, int screen) {
    if (!FcInit()) {
        fprintf(stderr, "FcInit failed\n");
        return false;
    }

    Visual *visual = DefaultVisual(dpy, screen);
    Colormap colormap = DefaultColormap(dpy, screen);
    
    // Allocate color assets
    if (!XftColorAllocName(dpy, visual, colormap, "#121212", &bgColor)) return false;
    if (!XftColorAllocName(dpy, visual, colormap, "#ffffff", &fontColor)) return false;
    if (!XftColorAllocName(dpy, visual, colormap, "#990000", &redColor)) return false;
    
    return true;
}

void clock_cleanup(Display *dpy, int screen) {
    Visual *visual = DefaultVisual(dpy, screen);
    Colormap colormap = DefaultColormap(dpy, screen);
    
    XftColorFree(dpy, visual, colormap, &bgColor);
    XftColorFree(dpy, visual, colormap, &fontColor);
    XftColorFree(dpy, visual, colormap, &redColor);

    // Free cached fonts
    if (cached_time_font) {
        XftFontClose(dpy, cached_time_font);
        cached_time_font = NULL;
    }
    if (cached_date_font) {
        XftFontClose(dpy, cached_date_font);
        cached_date_font = NULL;
    }
    cached_time_font_size = -1;
    cached_date_font_size = -1;

    // Free cached graphics assets
    if (cached_draw) {
        XftDrawDestroy(cached_draw);
        cached_draw = NULL;
    }
    if (cached_pixmap != None) {
        XFreePixmap(dpy, cached_pixmap);
        cached_pixmap = None;
    }
    cached_pixmap_width = -1;
    cached_pixmap_height = -1;

    FcFini();
}

void clock_zoom_in(void) {
    zoomScale += 0.1f;
    if (zoomScale > 10.0f) zoomScale = 10.0f;
    zoomChanged = true;
}

void clock_zoom_out(void) {
    zoomScale -= 0.1f;
    if (zoomScale < 0.1f) zoomScale = 0.1f;
    zoomChanged = true;
}

void clock_zoom_reset(void) {
    zoomScale = 1.0f;
    zoomChanged = true;
}

bool clock_draw(Display *dpy, Window win, int screen, int width, int height, bool force_redraw) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    bool timeChanged = (t->tm_min != cached_min || t->tm_hour != cached_hour || t->tm_mday != cached_mday);
    bool sizeChanged = (width != cached_pixmap_width || height != cached_pixmap_height);
    
    bool need_rebuild = (timeChanged || sizeChanged || zoomChanged || cached_zoom != zoomScale);
    
    if (need_rebuild) {
        cached_min = t->tm_min;
        cached_hour = t->tm_hour;
        cached_mday = t->tm_mday;
        cached_zoom = zoomScale;
        zoomChanged = false;

        // Recreate double-buffer Pixmap and XftDraw target only when dimensions change
        if (cached_pixmap == None || sizeChanged) {
            if (cached_draw) {
                XftDrawDestroy(cached_draw);
            }
            if (cached_pixmap != None) {
                XFreePixmap(dpy, cached_pixmap);
            }
            cached_pixmap = XCreatePixmap(dpy, win, width, height, DefaultDepth(dpy, screen));
            cached_draw = XftDrawCreate(dpy, cached_pixmap, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));
            cached_pixmap_width = width;
            cached_pixmap_height = height;
        }
        
        // Fill background
        XftDrawRect(cached_draw, &bgColor, 0, 0, width, height);
        
        int h = t->tm_hour % 12;
        if (h == 0) h = 12;
        int m = t->tm_min;
        
        char time_str[64];
        sprintf(time_str, "%02d:%02d", h, m);
        
        // Construct date segments (e.g. "Sun ", "08", " Jun")
        char day_of_week[32];
        char day_of_month[32];
        char month_name[32];
        strftime(day_of_week, sizeof(day_of_week), "%a", t);
        strftime(day_of_month, sizeof(day_of_month), "%d", t);
        strftime(month_name, sizeof(month_name), "%b", t);
        
        char part1[64];
        char part2[64];
        char part3[64];
        sprintf(part1, "%s ", day_of_week);
        sprintf(part2, "%s", day_of_month);
        sprintf(part3, " %s", month_name);
        
        // Calculate base font size for the time based on window height
        int base_font_size_time = (int)(height * 0.35f);
        if (base_font_size_time < 1) base_font_size_time = 1;
        
        XftFont *timeFont = get_time_font(dpy, screen, base_font_size_time);
        
        // Measure time text width
        XGlyphInfo extents_time;
        XftTextExtentsUtf8(dpy, timeFont, (const FcChar8*)time_str, strlen(time_str), &extents_time);
        float target_time_width = extents_time.xOff;
        
        // Clamp the base font size if it exceeds the window width safety margin (85%)
        if (target_time_width > width * 0.85f) {
            float fit_scale = (width * 0.85f) / target_time_width;
            base_font_size_time = (int)(base_font_size_time * fit_scale);
            if (base_font_size_time < 1) base_font_size_time = 1;
            timeFont = get_time_font(dpy, screen, base_font_size_time);
            
            XftTextExtentsUtf8(dpy, timeFont, (const FcChar8*)time_str, strlen(time_str), &extents_time);
            target_time_width = extents_time.xOff;
        }
        
        // Apply user chosen zoom scale
        int final_font_size_time = (int)(base_font_size_time * zoomScale);
        if (final_font_size_time < 1) final_font_size_time = 1;
        if (final_font_size_time != base_font_size_time) {
            timeFont = get_time_font(dpy, screen, final_font_size_time);
            XftTextExtentsUtf8(dpy, timeFont, (const FcChar8*)time_str, strlen(time_str), &extents_time);
            target_time_width = extents_time.xOff;
        }
        
        // Calculate base font size for the date string (30% of base time size)
        int base_font_size_date = (int)(base_font_size_time * 0.3f);
        if (base_font_size_date < 1) base_font_size_date = 1;
        XftFont *dateFont = get_date_font(dpy, screen, base_font_size_date);
        
        char full_date_str[256];
        sprintf(full_date_str, "%s%s%s", part1, part2, part3);
        
        XGlyphInfo extents_date;
        XftTextExtentsUtf8(dpy, dateFont, (const FcChar8*)full_date_str, strlen(full_date_str), &extents_date);
        float target_date_width = extents_date.xOff;
        
        // Clamp base date size if it exceeds safety margin
        if (target_date_width > width * 0.85f) {
            float fit_scale = (width * 0.85f) / target_date_width;
            base_font_size_date = (int)(base_font_size_date * fit_scale);
            if (base_font_size_date < 1) base_font_size_date = 1;
            dateFont = get_date_font(dpy, screen, base_font_size_date);
            
            XftTextExtentsUtf8(dpy, dateFont, (const FcChar8*)full_date_str, strlen(full_date_str), &extents_date);
            target_date_width = extents_date.xOff;
        }
        
        // Apply zoom scale to the final date font size
        int final_font_size_date = (int)(base_font_size_date * zoomScale);
        if (final_font_size_date < 1) final_font_size_date = 1;
        if (final_font_size_date != base_font_size_date) {
            dateFont = get_date_font(dpy, screen, final_font_size_date);
            XftTextExtentsUtf8(dpy, dateFont, (const FcChar8*)full_date_str, strlen(full_date_str), &extents_date);
            target_date_width = extents_date.xOff;
        }
        
        // Measure individual date segments
        XGlyphInfo ext1, ext2;
        XftTextExtentsUtf8(dpy, dateFont, (const FcChar8*)part1, strlen(part1), &ext1);
        XftTextExtentsUtf8(dpy, dateFont, (const FcChar8*)part2, strlen(part2), &ext2);
        
        // Height parameters
        float time_h = timeFont->ascent + timeFont->descent;
        float date_h = dateFont->ascent + dateFont->descent;
        
        // Centered alignment calculation
        float gap = height * 0.04f * zoomScale;
        float total_height = time_h + gap + date_h;
        float start_y = (height - total_height) / 2.0f;
        
        // Draw time text (using start_y + ascent for Xft vertical text baseline)
        float time_x = (width - target_time_width) / 2.0f;
        XftDrawStringUtf8(cached_draw, &fontColor, timeFont, (int)time_x, (int)(start_y + timeFont->ascent), (const FcChar8*)time_str, strlen(time_str));
        
        // Draw date segments
        float date_pos_y = start_y + time_h + gap;
        float date_x = (width - target_date_width) / 2.0f;
        
        XftDrawStringUtf8(cached_draw, &fontColor, dateFont, (int)date_x, (int)(date_pos_y + dateFont->ascent), (const FcChar8*)part1, strlen(part1));
        XftDrawStringUtf8(cached_draw, &redColor, dateFont, (int)(date_x + ext1.xOff), (int)(date_pos_y + dateFont->ascent), (const FcChar8*)part2, strlen(part2));
        XftDrawStringUtf8(cached_draw, &fontColor, dateFont, (int)(date_x + ext1.xOff + ext2.xOff), (int)(date_pos_y + dateFont->ascent), (const FcChar8*)part3, strlen(part3));
    }
    
    // Copy the double-buffered Pixmap to the visible window only if a change or Expose occurred
    if (need_rebuild || force_redraw) {
        GC gc = XCreateGC(dpy, win, 0, NULL);
        XCopyArea(dpy, cached_pixmap, win, gc, 0, 0, width, height, 0, 0);
        XFreeGC(dpy, gc);
        return true;
    }
    return false;
}
