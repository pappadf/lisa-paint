/* LisaPaint: a pixel painting tool for the Lisa Office System.
 *
 * The picture is a 1-bit bitmap the size of an 8 x 10 inch page at the
 * Lisa's resolution (90 x 60 dots per inch: 720 x 600 pixels; Lisa pixels
 * are half again as tall as they are wide).  It lives in an offscreen
 * QuickDraw port, so the tools draw into it with QuickDraw, and it is copied
 * to the window with CopyBits (scaled up when the pixels are magnified). */
#ifndef PAINT_H
#define PAINT_H

#include <stdint.h>
#include "lisa/desktop.h"

/* ---- the picture ------------------------------------------------------- */
enum { DOC_W = 720, DOC_H = 600, DOC_ROWBYTES = DOC_W / 8 };

/* The working bitmaps, all DOC_W x DOC_H.  `undo` holds the picture as it
 * was before the last change (Undo Last Change swaps the two), `base` the
 * picture without the floating selection while one is being moved. */
typedef struct {
    BitMap bits, undo, base;
    GrafPort port;                      /* offscreen port drawing into bits */
} Picture_;
extern Picture_ pic;

/* ---- tools, patterns, line widths, brushes ------------------------------ */
enum Tool {
    T_SELECT, T_BUCKET, T_SPRAY, T_BRUSH, T_PENCIL, T_ERASER, T_LINE,
    T_RECT, T_FRECT, T_RRECT, T_FRRECT, T_OVAL, T_FOVAL, T_COUNT
};
enum { N_PATTERNS = 38, N_LINES = 5, N_BRUSHES = 7 };

typedef struct {
    int tool, pattern, line, brush;
    int zoom;                           /* 1 or ZOOM */
    int sx, sy;                         /* picture point shown at the view's top left */
    int hasSel, floating;               /* selection; floating = lifted and movable */
    Rect sel;                           /* in picture coordinates */
    int dirty;                          /* changed since last saved */
    int canUndo;
} State;
extern State st;
enum { ZOOM = 4 };

extern const Pattern patterns[N_PATTERNS];
extern const Point lineSizes[N_LINES];  /* pen size (h, v) per line width */

/* ---- the window --------------------------------------------------------- */
extern WindowPtr win;                   /* the document window, 0 if none */
extern int active;                      /* our window is the active one */
extern Rect viewR;                      /* the picture view, window coordinates */

/* art.c */
void art_init(void);
void draw_tool_icon(int tool, const Rect *cell);
void set_tool_cursor(int tool);
extern const Pattern antPats[8];

/* view.c */
void view_init(void);
void view_layout(void);
void view_draw_all(void);
void view_show_doc(const Rect *docR);   /* copy part of the picture to the window */
void view_draw_palettes(void);
void view_activate(int on);
void view_set_thumbs(void);
void view_click(Point where, const EventRecord *ev);
void view_idle(void);
void view_scroll_to(int sx, int sy);
int  view_pt_to_doc(Point local, Point *doc);   /* 1 if inside the view */
Rect view_doc_to_win(const Rect *docR);
void view_ants(int erase);              /* draw (or erase) the selection outline */
void view_grow(Point where);
void view_set_zoom(int z);

/* main.c */
void menus_update(void);

/* tools.c */
void tools_init(void);
void tool_track(Point docPt, const EventRecord *ev);
void begin_change(void);                /* snapshot for Undo Last Change */
void commit_selection(void);
void drop_selection(void);
void cmd_undo(void);
void cmd_cut(void);
void cmd_copy(void);
void cmd_paste(void);
void cmd_clear(void);
void cmd_select_all(void);
void cmd_invert(void);
void cmd_fill(void);
void cmd_flip(int horizontal);
int  clip_empty(void);
void pic_clear(void);
void bits_copy(const BitMap *src, const BitMap *dst, const Rect *r);

/* doc.c: the document's diskfiles <prefix>PIC (saved) and <prefix>SUS
 * (suspended working state) */
enum { DOC_OK = 0, DOC_NONE, DOC_BAD };
int  doc_load(const unsigned char *prefix, int suspended);
int  doc_save(const unsigned char *prefix, int suspended);
void doc_remove(const unsigned char *prefix, int suspended);

/* alerts in the phrase file */
enum { A_REVERT = 1001, A_CANTSAVE, A_CANTREAD };
extern TAlertFile alertFile;

/* small helpers */
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int iabs(int a) { return a < 0 ? -a : a; }
static inline Point mkpt(int h, int v) { Point p; p.h = h; p.v = v; return p; }

#endif
