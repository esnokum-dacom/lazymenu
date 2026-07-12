#define _DEFAULT_SOURCE
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
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
static int mx = 0, my = 0;
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
    XMoveResizeWindow(d, win, mx + (mw - (winw + PAD_X)), my + (mh - (winh + PAD_Y)), winw, winh);
}

static Window get_active_window(Display *d, Window root) {
    Atom prop = XInternAtom(d, "_NET_ACTIVE_WINDOW", False);
    Atom type;
    int fmt;
    unsigned long n, rem;
    unsigned char *data = NULL;
    Window w = None;

    if (XGetWindowProperty(d, root, prop, 0, 1, False, XA_WINDOW,
        &type, &fmt, &n, &rem, &data) == Success && data) {
        w = *(Window *)data;
        XFree(data);
    }
    return w;
}

static int get_win_monitor(Display *d, Window w) {
    Atom prop = XInternAtom(d, "_SBCWM_MONITOR", False);
    Atom type;
    int fmt;
    unsigned long n, rem;
    unsigned char *data = NULL;

    if (XGetWindowProperty(d, w, prop, 0, 1, False, XA_CARDINAL,
        &type, &fmt, &n, &rem, &data) != Success || type != XA_CARDINAL || !data) {
        if (data) XFree(data);
        return -1;
    }

    int mon = (int)(*(unsigned long *)data);
    XFree(data);
    return mon;
}

static void render(void) {
    XClearWindow(d, win);

    if (selected >= 0 && selected < cur->count) {
        int y = PAD_Y + selected * ROW_H;
        XFillRectangle(d, win, selgc, 4, y, winw - 8, ROW_H);

        int cy = (PAD_Y * 2.5) + selected * ROW_H;
        XftDrawStringUtf8(draw, &fg, font, 4, cy,
                           (FcChar8 *)"|", strlen("|"));

    }

    for (int i = 0; i < cur->count; i++) {
        const Bind *b = &cur->items[i];
        const char *key = XKeysymToString(b->key);
        int y = PAD_Y + (i + 1) * ROW_H - 6;
        XftDrawStringUtf8(draw, &hl, font, PAD_X, y,
                           (FcChar8 *)key, strlen(key));
        XftDrawStringUtf8(draw, &fg, font, PAD_X + KEY_COL, y,
                           (FcChar8 *)b->label, strlen(b->label));
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

    Window active = get_active_window(d, root);
    int target_mon = -1;
    if (active != None) {
        target_mon = get_win_monitor(d, active);
    }

    int nmon = 0;
    XRRMonitorInfo *info = XRRGetMonitors(d, root, True, &nmon);
    if (info && nmon > 0) {
        if (target_mon >= 0 && target_mon < nmon) {
            mx = info[target_mon].x;
            my = info[target_mon].y;
            mw = info[target_mon].width;
            mh = info[target_mon].height;
        }
        XRRFreeMonitors(info);
    }

    font = XftFontOpenName(d, scr, FONT);
    if (!font) { fprintf(stderr, "cannot load font\n"); return 1; }

    cur = &root_menu;
    size_for(cur, &winw, &winh);

    XSetWindowAttributes attrs = {0};
    attrs.override_redirect = True;
    attrs.background_pixel = BlackPixel(d, scr);
    attrs.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask;

    win = XCreateWindow(d, root, mx + (mw - (winw + PAD_X)), my + (mh - (winh + PAD_Y)), winw, winh, 1,
                         DefaultDepth(d, scr), InputOutput, vis,
                         CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);
    XStoreName(d, win, "lazymenu");
    XMapRaised(d, win);

    /* Wait until the window is actually viewable before grabbing the
     * keyboard. Grabbing immediately after XMapRaised() races the X
     * server: the grab can be issued before the map has taken effect,
     * silently fails (return value was previously ignored), and the
     * first keypress(es) get lost. */
    {
        XEvent me;
        do {
            XNextEvent(d, &me);
        } while (me.type != MapNotify || me.xmap.window != win);
    }

    {
        int gresult;
        int tries = 0;
        do {
            gresult = XGrabKeyboard(d, win, True, GrabModeAsync,
                                     GrabModeAsync, CurrentTime);
            if (gresult != GrabSuccess) usleep(1000);
        } while (gresult != GrabSuccess && ++tries < 100);
        if (gresult != GrabSuccess)
            fprintf(stderr, "lazymenu: failed to grab keyboard\n");
    }

    draw = XftDrawCreate(d, win, vis, cmap);
    XRenderColor fgc = { 0xcccc, 0xcccc, 0xcccc, 0xffff };
    XRenderColor hlc = { 0xffff, 0xaaaa, 0x0000, 0xffff };
    XftColorAllocValue(d, vis, cmap, &fgc, &fg);
    XftColorAllocValue(d, vis, cmap, &hlc, &hl);

    XftColorAllocValue(d, vis, cmap, &hlc, &hl);

    XColor selc;
    XAllocNamedColor(d, cmap, SEL_BG, &selc, &selc);
    selgc = XCreateGC(d, win, 0, NULL);
    XSetForeground(d, selgc, selc.pixel);

    render();

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
