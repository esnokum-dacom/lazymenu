#define _DEFAULT_SOURCE
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct Bind Bind;

typedef struct {
    const Bind *items;
    int count;
} Menu;

struct Bind {
    KeySym key;
    const char *label;
    const char *cmd;
    const Menu *submenu;
};

#include "config.h"

#define FONT       "monospace-12"
#define PAD_X      16
#define PAD_Y      10
#define ROW_H      22
#define KEY_COL    30
#define MAX_DEPTH  8
#define SEL_BG     "#3a3a3a"

static Display *d;
static Window win;
static XftDraw *draw;
static XftFont *font;
static XftColor fg, hl;
static GC selgc;
static int scr, mw, mh;
static int winw, winh;
static Visual *vis;
static Colormap cmap;

static const Menu *stack[MAX_DEPTH];
static int depth = 0;
static const Menu *cur;
static int selected = 0;

static void size_for(const Menu *m, int *w, int *h) {
    int maxw = 0;
    for (int i = 0; i < m->count; i++) {
        XGlyphInfo ext;
        XftTextExtentsUtf8(d, font, (FcChar8 *)m->items[i].label,
                            strlen(m->items[i].label), &ext);
        if (ext.xOff > maxw) maxw = ext.xOff;
    }
    *w = maxw + KEY_COL + PAD_X * 2;
    *h = m->count * ROW_H + PAD_Y * 2;
}

static void resize_for_menu(const Menu *m) {
    size_for(m, &winw, &winh);
    XMoveResizeWindow(d, win, (mw - (winw + PAD_X)), (mh - (winh + PAD_Y)), winw, winh);
}

static void render(void) {
    XClearWindow(d, win);

    if (selected >= 0 && selected < cur->count) {
        int y = PAD_Y + selected * ROW_H;
        XFillRectangle(d, win, selgc, 4, y, winw - 8, ROW_H);
    }

    for (int i = 0; i < cur->count; i++) {
        const Bind *b = &cur->items[i];
        const char *key = XKeysymToString(b->key);
        int y = PAD_Y + (i + 1) * ROW_H - 6;
        XftDrawStringUtf8(draw, &hl, font, PAD_X, y,
                           (FcChar8 *)key, strlen(key));
        XftDrawStringUtf8(draw, &fg, font, PAD_X + KEY_COL, y,
                           (FcChar8 *)b->label, strlen(b->label));
        if (b->submenu) {
            int x = PAD_X + KEY_COL + 200;
            XftDrawStringUtf8(draw, &fg, font, x, y, (FcChar8 *)">", 1);
        }
    }
}

static void enter_menu(const Menu *m) {
    cur = m;
    selected = 0;
    resize_for_menu(cur);
}

static void activate(const Bind *b) {
    if (b->submenu) {
        stack[depth++] = cur;
        enter_menu(b->submenu);
        render();
    } else if (b->cmd) {
        XUngrabKeyboard(d, CurrentTime);
        XDestroyWindow(d, win);
        XCloseDisplay(d);
        execlp(defaultsh, "sh", "-c", b->cmd, (char *)NULL);
        _exit(127);
    }
}

static void go_back(void) {
    if (depth == 0) return;
    cur = stack[--depth];
    selected = 0;
    resize_for_menu(cur);
    render();
}

int main(void) {
    d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "cannot open display\n"); return 1; }

    scr = DefaultScreen(d);
    Window root = RootWindow(d, scr);
    vis = DefaultVisual(d, scr);
    cmap = DefaultColormap(d, scr);
    mw = DisplayWidth(d, scr);
    mh = DisplayHeight(d, scr);

    font = XftFontOpenName(d, scr, FONT);
    if (!font) { fprintf(stderr, "cannot load font\n"); return 1; }

    cur = &root_menu;
    size_for(cur, &winw, &winh);

    XSetWindowAttributes attrs = {0};
    attrs.override_redirect = True;
    attrs.background_pixel = BlackPixel(d, scr);
    attrs.event_mask = KeyPressMask | ExposureMask;

    win = XCreateWindow(d, root, (mw - (winw + PAD_X)), (mh - (winh + PAD_Y)), winw, winh, 1,
                         DefaultDepth(d, scr), InputOutput, vis,
                         CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);
    XStoreName(d, win, "lazymenu");
    XMapRaised(d, win);

    draw = XftDrawCreate(d, win, vis, cmap);
    XRenderColor fgc = { 0xcccc, 0xcccc, 0xcccc, 0xffff };
    XRenderColor hlc = { 0xffff, 0xaaaa, 0x0000, 0xffff };
    XftColorAllocValue(d, vis, cmap, &fgc, &fg);
    XftColorAllocValue(d, vis, cmap, &hlc, &hl);

    XColor selc;
    XAllocNamedColor(d, cmap, SEL_BG, &selc, &selc);
    selgc = XCreateGC(d, win, 0, NULL);
    XSetForeground(d, selgc, selc.pixel);

    XGrabKeyboard(d, win, True, GrabModeAsync, GrabModeAsync, CurrentTime);

    int running = 1;
    while (running) {
        XEvent e;
        XNextEvent(d, &e);

        if (e.type == Expose) {
            render();
            continue;
        }
        if (e.type != KeyPress) continue;

        KeySym ks = XLookupKeysym(&e.xkey, 0);

        if (ks == XK_Escape) { running = 0; continue; }

        if (ks == XK_Down) {
            if (selected < cur->count - 1)
		selected++;
            render();
            continue;
        }
        if (ks == XK_Up) {
            if (selected > 0) selected--;
            render();
            continue;
        }
        if (ks == XK_Return || ks == XK_Right) {
            activate(&cur->items[selected]);
            continue;
        }
        if (ks == XK_BackSpace || ks == XK_Left) {
            go_back();
            continue;
        }

        for (int i = 0; i < cur->count; i++) {
            if (ks == cur->items[i].key) {
                selected = i;
                activate(&cur->items[i]);
                break;
            }
        }
    }

    XUngrabKeyboard(d, CurrentTime);
    XFreeGC(d, selgc);
    XftColorFree(d, vis, cmap, &fg);
    XftColorFree(d, vis, cmap, &hl);
    XftDrawDestroy(draw);
    XftFontClose(d, font);
    XDestroyWindow(d, win);
    XCloseDisplay(d);
    return 0;
}
