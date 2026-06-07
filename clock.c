#include "clock.h"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static float zoomScale = 1.0f;

static XftColor bgColor;
static XftColor fontColor;
static XftColor redColor;

bool clock_init(Display *dpy, int screen) {
    Visual *visual = DefaultVisual(dpy, screen);
    Colormap colormap = DefaultColormap(dpy, screen);
    
    // Allocate our color assets
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
}

void clock_zoom_in(void) {
    zoomScale += 0.1f;
    if (zoomScale > 10.0f) zoomScale = 10.0f;
}

void clock_zoom_out(void) {
    zoomScale -= 0.1f;
    if (zoomScale < 0.1f) zoomScale = 0.1f;
}

void clock_zoom_reset(void) {
    zoomScale = 1.0f;
}

void clock_draw(Display *dpy, Window win, int screen, int width, int height) {
    // Create double-buffer Pixmap to prevent window flickers
    Pixmap pix = XCreatePixmap(dpy, win, width, height, DefaultDepth(dpy, screen));
    
    // Create XftDraw target for the Pixmap
    XftDraw *draw = XftDrawCreate(dpy, pix, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));
    
    // Fill background
    XftDrawRect(draw, &bgColor, 0, 0, width, height);
    
    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
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
    float base_font_size_time = height * 0.35f;
    
    // Open bold time font using Xft
    XftFont *timeFont = NULL;
    char font_query[512];
    
    sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", (int)base_font_size_time);
    timeFont = XftFontOpenName(dpy, screen, font_query);
    if (!timeFont) {
        sprintf(font_query, "monospace:style=Bold:pixelsize=%d", (int)base_font_size_time);
        timeFont = XftFontOpenName(dpy, screen, font_query);
    }
    
    // Measure time text width
    XGlyphInfo extents_time;
    XftTextExtentsUtf8(dpy, timeFont, (const FcChar8*)time_str, strlen(time_str), &extents_time);
    float target_time_width = extents_time.xOff;
    
    // Clamp the base font size if it exceeds the window width safety margin (85%)
    if (target_time_width > width * 0.85f) {
        float fit_scale = (width * 0.85f) / target_time_width;
        base_font_size_time *= fit_scale;
        XftFontClose(dpy, timeFont);
        
        sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", (int)base_font_size_time);
        timeFont = XftFontOpenName(dpy, screen, font_query);
        if (!timeFont) {
            sprintf(font_query, "monospace:style=Bold:pixelsize=%d", (int)base_font_size_time);
            timeFont = XftFontOpenName(dpy, screen, font_query);
        }
        
        XftTextExtentsUtf8(dpy, timeFont, (const FcChar8*)time_str, strlen(time_str), &extents_time);
        target_time_width = extents_time.xOff;
    }
    
    // Apply user chosen zoom scale
    float final_font_size_time = base_font_size_time * zoomScale;
    if (final_font_size_time != base_font_size_time) {
        XftFontClose(dpy, timeFont);
        sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", (int)final_font_size_time);
        timeFont = XftFontOpenName(dpy, screen, font_query);
        if (!timeFont) {
            sprintf(font_query, "monospace:style=Bold:pixelsize=%d", (int)final_font_size_time);
            timeFont = XftFontOpenName(dpy, screen, font_query);
        }
        XftTextExtentsUtf8(dpy, timeFont, (const FcChar8*)time_str, strlen(time_str), &extents_time);
        target_time_width = extents_time.xOff;
    }
    
    // Calculate base font size for the date string (30% of base time size)
    float base_font_size_date = base_font_size_time * 0.3f;
    XftFont *dateFont = NULL;
    sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", (int)base_font_size_date);
    dateFont = XftFontOpenName(dpy, screen, font_query);
    if (!dateFont) {
        sprintf(font_query, "monospace:style=Bold:pixelsize=%d", (int)base_font_size_date);
        dateFont = XftFontOpenName(dpy, screen, font_query);
    }
    
    char full_date_str[256];
    sprintf(full_date_str, "%s%s%s", part1, part2, part3);
    
    XGlyphInfo extents_date;
    XftTextExtentsUtf8(dpy, dateFont, (const FcChar8*)full_date_str, strlen(full_date_str), &extents_date);
    float target_date_width = extents_date.xOff;
    
    // Clamp base date size if it exceeds safety margin
    if (target_date_width > width * 0.85f) {
        float fit_scale = (width * 0.85f) / target_date_width;
        base_font_size_date *= fit_scale;
        XftFontClose(dpy, dateFont);
        
        sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", (int)base_font_size_date);
        dateFont = XftFontOpenName(dpy, screen, font_query);
        if (!dateFont) {
            sprintf(font_query, "monospace:style=Bold:pixelsize=%d", (int)base_font_size_date);
            dateFont = XftFontOpenName(dpy, screen, font_query);
        }
        
        XftTextExtentsUtf8(dpy, dateFont, (const FcChar8*)full_date_str, strlen(full_date_str), &extents_date);
        target_date_width = extents_date.xOff;
    }
    
    // Apply zoom scale to the final date font size
    float final_font_size_date = base_font_size_date * zoomScale;
    if (final_font_size_date != base_font_size_date) {
        XftFontClose(dpy, dateFont);
        sprintf(font_query, "RecMonoSmCasual Nerd Font:style=Bold:pixelsize=%d", (int)final_font_size_date);
        dateFont = XftFontOpenName(dpy, screen, font_query);
        if (!dateFont) {
            sprintf(font_query, "monospace:style=Bold:pixelsize=%d", (int)final_font_size_date);
            dateFont = XftFontOpenName(dpy, screen, font_query);
        }
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
    XftDrawStringUtf8(draw, &fontColor, timeFont, (int)time_x, (int)(start_y + timeFont->ascent), (const FcChar8*)time_str, strlen(time_str));
    
    // Draw date segments
    float date_pos_y = start_y + time_h + gap;
    float date_x = (width - target_date_width) / 2.0f;
    
    XftDrawStringUtf8(draw, &fontColor, dateFont, (int)date_x, (int)(date_pos_y + dateFont->ascent), (const FcChar8*)part1, strlen(part1));
    XftDrawStringUtf8(draw, &redColor, dateFont, (int)(date_x + ext1.xOff), (int)(date_pos_y + dateFont->ascent), (const FcChar8*)part2, strlen(part2));
    XftDrawStringUtf8(draw, &fontColor, dateFont, (int)(date_x + ext1.xOff + ext2.xOff), (int)(date_pos_y + dateFont->ascent), (const FcChar8*)part3, strlen(part3));
    
    // Swap buffer by copying Pixmap to window
    GC gc = XCreateGC(dpy, win, 0, NULL);
    XCopyArea(dpy, pix, win, gc, 0, 0, width, height, 0, 0);
    XFreeGC(dpy, gc);
    
    // Free fonts and graphics objects
    XftFontClose(dpy, timeFont);
    XftFontClose(dpy, dateFont);
    XftDrawDestroy(draw);
    XFreePixmap(dpy, pix);
}
