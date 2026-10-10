/* The painting tools, the selection, Undo Last Change and the clipboard.
 *
 * Every change starts with begin_change(), which copies the picture to the
 * undo bitmap.  Shapes are "rubber-banded": while the mouse moves, the area
 * of the previous outline is restored from the undo bitmap and the shape is
 * drawn again.  Shift constrains lines to 0/45/90 degrees and rectangles
 * and ovals to squares and circles -- as they look on the screen, where a
 * pixel is half again as tall as it is wide. */
#include <stdlib.h>
#include <string.h>
#include "paint.h"

Picture_ pic;
State st;

static BitMap floatBits;                /* the lifted selection */
static BitMap clipBits;                 /* LisaPaint's clipboard */
static uint8_t *visited;                /* paint bucket scratch */

enum { DOC_BYTES = DOC_ROWBYTES * DOC_H };

static void bitmap_init(BitMap *b, int w, int h)
{
    b->rowBytes = (Integer)(((w + 15) / 16) * 2);
    b->baseAddr = (QDPtr)calloc(1, (size_t)b->rowBytes * h);
    SetRect(&b->bounds, 0, 0, w, h);
}

static void bitmap_free(BitMap *b)
{
    free(b->baseAddr);
    b->baseAddr = 0;
}

void tools_init(void)
{
    Rect r;
    bitmap_init(&pic.bits, DOC_W, DOC_H);
    bitmap_init(&pic.undo, DOC_W, DOC_H);
    bitmap_init(&pic.base, DOC_W, DOC_H);
    visited = malloc(DOC_BYTES);
    OpenPort(&pic.port);
    SetPortBits(&pic.bits);
    PortSize(DOC_W, DOC_H);
    SetRect(&r, 0, 0, DOC_W, DOC_H);
    RectRgn(pic.port.visRgn, &r);
    ClipRect(&r);
    st.zoom = 1;
    st.line = 1;
    st.tool = T_PENCIL;
}

void pic_clear(void)
{
    memset(pic.bits.baseAddr, 0, DOC_BYTES);
    memset(pic.undo.baseAddr, 0, DOC_BYTES);
    st.canUndo = 0;
    st.hasSel = st.floating = 0;
}

/* Copy rectangle r (same place in both) between two picture-sized bitmaps. */
void bits_copy(const BitMap *src, const BitMap *dst, const Rect *r)
{
    SetPort(&pic.port);
    CopyBits(src, dst, r, r, srcCopy, 0);
}

static inline int px(const BitMap *b, int x, int y)
{
    return (((uint8_t *)b->baseAddr)[y * b->rowBytes + (x >> 3)] >> (7 - (x & 7))) & 1;
}

static inline void setpx(const BitMap *b, int x, int y, int on)
{
    uint8_t *p = (uint8_t *)b->baseAddr + y * b->rowBytes + (x >> 3);
    if (on) *p |= 0x80 >> (x & 7);
    else    *p &= ~(0x80 >> (x & 7));
}

static inline int patpx(int x, int y)
{
    return (patterns[st.pattern].pat[y & 7] >> (7 - (x & 7))) & 1;
}

void begin_change(void)
{
    memcpy(pic.undo.baseAddr, pic.bits.baseAddr, DOC_BYTES);
    st.canUndo = 1;
    st.dirty = 1;
}

void commit_selection(void)
{
    if (st.floating) {
        bitmap_free(&floatBits);
        st.floating = 0;
    }
}

void drop_selection(void)
{
    if (!st.hasSel)
        return;
    view_ants(1);
    commit_selection();
    st.hasSel = 0;
}

static void show_all(void)
{
    Rect r;
    SetRect(&r, 0, 0, DOC_W, DOC_H);
    view_show_doc(&r);
}

/* ---- mouse tracking ------------------------------------------------------- */

static Point mouse_doc(void)
{
    Point m, d;
    SetPort(win);
    GetMouse(&m);
    view_pt_to_doc(m, &d);
    return d;
}

static Point clamp_doc(Point p)
{
    p.h = imax(0, imin(p.h, DOC_W - 1));
    p.v = imax(0, imin(p.v, DOC_H - 1));
    return p;
}

static Rect rect_of(Point a, Point b)
{
    Rect r;
    SetRect(&r, imin(a.h, b.h), imin(a.v, b.v), imax(a.h, b.h) + 1, imax(a.v, b.v) + 1);
    return r;
}

static Rect grow_rect(Rect r, int dh, int dv)
{
    r.left -= dh; r.top -= dv; r.right += dh; r.bottom += dv;
    return r;
}

static Rect union_rect(Rect a, Rect b)
{
    if (a.left >= a.right) return b;
    if (b.left >= b.right) return a;
    UnionRect(&a, &b, &a);
    return a;
}

/* Shift: lines at 0, 45 or 90 degrees on the screen (a 45 degree line goes
 * 3 pixels across for every 2 down). */
static Point constrain_line(Point a, Point b)
{
    int dx = b.h - a.h, dy = b.v - a.v;
    int px2 = iabs(dx) * 2, py3 = iabs(dy) * 3;   /* compare on-screen lengths */
    if (py3 * 5 < px2 * 2)       b.v = a.v;        /* nearly horizontal */
    else if (px2 * 5 < py3 * 2)  b.h = a.h;        /* nearly vertical */
    else {
        int len = imax(px2, py3) / 6;              /* steps of (3, 2) */
        b.h = a.h + (dx < 0 ? -3 : 3) * len;
        b.v = a.v + (dy < 0 ? -2 : 2) * len;
    }
    return b;
}

/* Shift: squares and circles on the screen (3 wide for every 2 tall). */
static Point constrain_square(Point a, Point b)
{
    int dx = b.h - a.h, dy = b.v - a.v;
    int n = imax(iabs(dx) / 3, iabs(dy) / 2);
    b.h = a.h + (dx < 0 ? -3 : 3) * n;
    b.v = a.v + (dy < 0 ? -2 : 2) * n;
    return b;
}

/* ---- freehand tools -------------------------------------------------------- */

static void stamp(int x, int y, Rect *bounds)
{
    Rect r;
    switch (st.tool) {
    case T_ERASER:
        SetRect(&r, x - 8, y - 5, x + 8, y + 6);      /* 16 x 11: square on screen */
        EraseRect(&r);
        break;
    case T_BRUSH:
        PenNormal();
        switch (st.brush) {
        case 0: SetRect(&r, x - 2, y - 1, x + 2, y + 2); FillOval(&r, &patterns[st.pattern]); break;
        case 1: SetRect(&r, x - 4, y - 3, x + 5, y + 3); FillOval(&r, &patterns[st.pattern]); break;
        case 2: SetRect(&r, x - 7, y - 5, x + 8, y + 5); FillOval(&r, &patterns[st.pattern]); break;
        case 3: SetRect(&r, x - 4, y - 3, x + 5, y + 3); FillRect(&r, &patterns[st.pattern]); break;
        default: {
            static const int dx[] = {0, 0, 0, 0, 5, 7, 0}, dy[] = {0, 0, 0, 0, -4, 0, 5};
            int ex = dx[st.brush], ey = dy[st.brush];
            PenPat(&patterns[st.pattern]);
            PenSize(2, 1);
            MoveTo(x - ex, y - ey);
            LineTo(x + ex, y + ey);
            PenNormal();
            SetRect(&r, x - 8, y - 6, x + 10, y + 7);
            break;
        }
        }
        break;
    default:
        return;
    }
    *bounds = union_rect(*bounds, grow_rect(r, 1, 1));
}

static void stroke(Point a, Point b, Rect *bounds)
{
    int dx = b.h - a.h, dy = b.v - a.v;
    int n = imax(iabs(dx), iabs(dy) * 3 / 2) / 2 + 1;
    for (int i = 1; i <= n; i++)
        stamp(a.h + dx * i / n, a.v + dy * i / n, bounds);
}

static void track_freehand(Point p)
{
    Point last = p, cur;
    Rect bounds = {0, 0, 0, 0};
    int color = 1;
    SetPort(&pic.port);
    if (st.tool == T_PENCIL) {
        color = !px(&pic.bits, p.h, p.v);   /* starting on black draws white */
        PenNormal();
        PenPat(color ? &patterns[0] : &patterns[6]);
        MoveTo(p.h, p.v);
        LineTo(p.h, p.v);
        bounds = grow_rect(rect_of(p, p), 1, 1);
    } else {
        stamp(p.h, p.v, &bounds);
    }
    view_show_doc(&bounds);
    while (StillDown()) {
        cur = mouse_doc();
        if (cur.h == last.h && cur.v == last.v)
            continue;
        bounds.left = bounds.right = 0;
        SetPort(&pic.port);
        if (st.tool == T_PENCIL) {
            LineTo(cur.h, cur.v);
            bounds = grow_rect(rect_of(last, cur), 1, 1);
        } else {
            stroke(last, cur, &bounds);
        }
        view_show_doc(&bounds);
        last = cur;
    }
    PenNormal();
}

static void track_spray(Point p)
{
    long next = 0;
    while (1) {
        Rect r;
        if (Time() < next) {            /* a steady flow: 8 drops every 1/50 s */
            if (!StillDown())
                break;
            p = mouse_doc();
            continue;
        }
        next = Time() + 2;
        for (int i = 0; i < 8; i++) {
            int rx = Random() % 12, ry = Random() % 8;
            int x = p.h + rx, y = p.v + ry;
            if (rx * rx * 4 + ry * ry * 9 > 576)        /* inside a 24 x 16 ellipse */
                continue;
            if (x >= 0 && x < DOC_W && y >= 0 && y < DOC_H && patpx(x, y))
                setpx(&pic.bits, x, y, 1);
        }
        SetRect(&r, p.h - 12, p.v - 8, p.h + 13, p.v + 9);
        view_show_doc(&r);
        if (!StillDown())
            break;
        p = mouse_doc();
    }
}

/* Paint bucket: fill the area of like-colored pixels around p with the
 * pattern (scan-line flood fill). */
static void bucket(Point p)
{
    enum { STACK = 8192 };
    static int16_t stack[STACK][2];
    int sp = 0, target, x0, x1;
    Rect bounds;
    if (p.h < 0 || p.h >= DOC_W || p.v < 0 || p.v >= DOC_H)
        return;
    target = px(&pic.bits, p.h, p.v);
    memset(visited, 0, DOC_BYTES);
    SetRect(&bounds, p.h, p.v, p.h + 1, p.v + 1);
    stack[sp][0] = p.h; stack[sp][1] = p.v; sp++;
#define SEEN(x, y) ((visited[(y) * DOC_ROWBYTES + ((x) >> 3)] >> (7 - ((x) & 7))) & 1)
#define OPEN(x, y) (!SEEN(x, y) && px(&pic.bits, x, y) == target)
    while (sp) {
        int x, y;
        sp--;
        x = stack[sp][0]; y = stack[sp][1];
        if (!OPEN(x, y))
            continue;
        x0 = x1 = x;
        while (x0 > 0 && OPEN(x0 - 1, y)) x0--;
        while (x1 < DOC_W - 1 && OPEN(x1 + 1, y)) x1++;
        for (int i = x0; i <= x1; i++) {
            visited[y * DOC_ROWBYTES + (i >> 3)] |= 0x80 >> (i & 7);
            setpx(&pic.bits, i, y, patpx(i, y));
        }
        bounds.left = imin(bounds.left, x0);
        bounds.right = imax(bounds.right, x1 + 1);
        bounds.top = imin(bounds.top, y);
        bounds.bottom = imax(bounds.bottom, y + 1);
        for (int ny = y - 1; ny <= y + 1; ny += 2) {
            int in = 0;
            if (ny < 0 || ny >= DOC_H)
                continue;
            for (int i = x0; i <= x1; i++) {
                if (OPEN(i, ny)) {
                    if (!in && sp < STACK) {
                        stack[sp][0] = i; stack[sp][1] = ny; sp++;
                    }
                    in = 1;
                } else {
                    in = 0;
                }
            }
        }
    }
#undef SEEN
#undef OPEN
    view_show_doc(&bounds);
}

/* ---- lines and shapes -------------------------------------------------------- */

static void draw_shape(Point a, Point b, Rect *bounds)
{
    Point pen = lineSizes[st.line];
    Rect r;
    SetPort(&pic.port);
    PenNormal();
    PenSize(pen.h, pen.v);
    if (st.tool == T_LINE) {
        MoveTo(a.h, a.v);
        LineTo(b.h, b.v);
        *bounds = grow_rect(rect_of(a, b), pen.h + 1, pen.v + 1);
        PenNormal();
        return;
    }
    r = rect_of(a, b);
    switch (st.tool) {
    case T_RECT:   FrameRect(&r); break;
    case T_FRECT:  FillRect(&r, &patterns[st.pattern]); FrameRect(&r); break;
    case T_RRECT:  FrameRoundRect(&r, 24, 16); break;
    case T_FRRECT: FillRoundRect(&r, 24, 16, &patterns[st.pattern]); FrameRoundRect(&r, 24, 16); break;
    case T_OVAL:   FrameOval(&r); break;
    case T_FOVAL:  FillOval(&r, &patterns[st.pattern]); FrameOval(&r); break;
    }
    PenNormal();
    *bounds = grow_rect(r, 1, 1);
}

static void track_shape(Point a, int shift)
{
    Rect prev = {0, 0, 0, 0}, now;
    Point b = a, last = {-1, -1};
    do {
        b = mouse_doc();
        if (shift)
            b = st.tool == T_LINE ? constrain_line(a, b) : constrain_square(a, b);
        if (b.h == last.h && b.v == last.v)
            continue;
        last = b;
        if (prev.left < prev.right)
            bits_copy(&pic.undo, &pic.bits, &prev);
        draw_shape(a, b, &now);
        {
            Rect u = union_rect(prev, now);
            view_show_doc(&u);
        }
        prev = now;
    } while (StillDown());
}

/* ---- the selection ---------------------------------------------------------- */

static void lift_selection(void)
{
    Rect src = st.sel, dst;
    begin_change();
    bitmap_init(&floatBits, st.sel.right - st.sel.left, st.sel.bottom - st.sel.top);
    dst = floatBits.bounds;
    SetPort(&pic.port);
    CopyBits(&pic.bits, &floatBits, &src, &dst, srcCopy, 0);
    memcpy(pic.base.baseAddr, pic.bits.baseAddr, DOC_BYTES);
    SetPort(&pic.port);
    {
        BitMap save = pic.port.portBits;
        SetPortBits(&pic.base);
        EraseRect(&src);
        SetPortBits(&save);
    }
    st.floating = 1;
}

/* Put the floating selection at r (restoring what was under its old place). */
static void place_float(Rect old, Rect r)
{
    Rect u = union_rect(old, r), src = floatBits.bounds;
    bits_copy(&pic.base, &pic.bits, &u);
    SetPort(&pic.port);
    CopyBits(&floatBits, &pic.bits, &src, &r, srcCopy, 0);
    st.sel = r;
    view_show_doc(&u);
}

static void track_move(Point p)
{
    Rect start = st.sel;
    Point last = p;
    if (!st.floating)
        lift_selection();
    view_ants(1);
    while (StillDown()) {
        Point cur = mouse_doc();
        Rect r = start;
        if (cur.h == last.h && cur.v == last.v)
            continue;
        last = cur;
        OffsetRect(&r, cur.h - p.h, cur.v - p.v);
        /* keep at least a corner of it on the picture */
        if (r.right < 8 || r.bottom < 8 || r.left > DOC_W - 8 || r.top > DOC_H - 8)
            continue;
        st.hasSel = 0;
        place_float(st.sel, r);
        st.hasSel = 1;
    }
    view_ants(0);
}

static void track_marquee(Point a, int shift)
{
    Point b, last = {-1, -1};
    drop_selection();
    do {
        b = clamp_doc(mouse_doc());
        if (shift)
            b = clamp_doc(constrain_square(a, b));
        if (b.h == last.h && b.v == last.v)
            continue;
        last = b;
        view_ants(1);                   /* erase the old outline, if any */
        st.sel = rect_of(a, b);
        st.hasSel = 1;
        view_ants(0);
    } while (StillDown());
    if (st.sel.right - st.sel.left < 2 || st.sel.bottom - st.sel.top < 2) {
        view_ants(1);                   /* a click: no selection */
        st.hasSel = 0;
    }
}

void tool_track(Point p, const EventRecord *ev)
{
    if (st.tool == T_SELECT) {
        if (st.hasSel && PtInRect(p, &st.sel))
            track_move(p);
        else
            track_marquee(clamp_doc(p), ev->shiftKey);
        return;
    }
    drop_selection();
    begin_change();
    switch (st.tool) {
    case T_PENCIL:
    case T_BRUSH:
    case T_ERASER: track_freehand(p); break;
    case T_SPRAY:  track_spray(p); break;
    case T_BUCKET: bucket(p); break;
    default:       track_shape(p, ev->shiftKey); break;
    }
}

/* ---- Edit and Selection menu commands --------------------------------------- */

void cmd_undo(void)
{
    if (!st.canUndo)
        return;
    drop_selection();
    memcpy(pic.base.baseAddr, pic.bits.baseAddr, DOC_BYTES);
    memcpy(pic.bits.baseAddr, pic.undo.baseAddr, DOC_BYTES);
    memcpy(pic.undo.baseAddr, pic.base.baseAddr, DOC_BYTES);
    st.dirty = 1;
    show_all();
}

int clip_empty(void) { return clipBits.baseAddr == 0; }

void cmd_copy(void)
{
    Rect dst;
    if (!st.hasSel)
        return;
    bitmap_free(&clipBits);
    bitmap_init(&clipBits, st.sel.right - st.sel.left, st.sel.bottom - st.sel.top);
    dst = clipBits.bounds;
    SetPort(&pic.port);
    CopyBits(&pic.bits, &clipBits, &st.sel, &dst, srcCopy, 0);
}

void cmd_clear(void)
{
    Rect r;
    if (!st.hasSel)
        return;
    r = st.sel;
    commit_selection();
    view_ants(1);
    st.hasSel = 0;
    begin_change();
    SetPort(&pic.port);
    EraseRect(&r);
    view_show_doc(&r);
}

void cmd_cut(void)
{
    cmd_copy();
    cmd_clear();
}

void cmd_paste(void)
{
    Rect r;
    int w, h, x, y;
    if (clip_empty())
        return;
    drop_selection();
    w = clipBits.bounds.right;
    h = clipBits.bounds.bottom;
    /* near the top left of what is in view, but on the picture */
    x = imax(0, imin(st.sx + 8 / st.zoom, DOC_W - w));
    y = imax(0, imin(st.sy + 8 / st.zoom, DOC_H - h));
    SetRect(&r, x, y, x + w, y + h);
    begin_change();
    memcpy(pic.base.baseAddr, pic.bits.baseAddr, DOC_BYTES);
    bitmap_init(&floatBits, w, h);
    memcpy(floatBits.baseAddr, clipBits.baseAddr, (size_t)clipBits.rowBytes * h);
    st.floating = 1;
    place_float(r, r);
    st.hasSel = 1;
    st.tool = T_SELECT;
    view_draw_palettes();
    view_ants(0);
}

void cmd_select_all(void)
{
    drop_selection();
    SetRect(&st.sel, 0, 0, DOC_W, DOC_H);
    st.hasSel = 1;
    st.tool = T_SELECT;
    view_draw_palettes();
    view_ants(0);
}

static void sel_change(void)
{
    commit_selection();
    begin_change();
    SetPort(&pic.port);
}

void cmd_invert(void)
{
    if (!st.hasSel) return;
    sel_change();
    InvertRect(&st.sel);
    view_show_doc(&st.sel);
}

void cmd_fill(void)
{
    if (!st.hasSel) return;
    sel_change();
    FillRect(&st.sel, &patterns[st.pattern]);
    view_show_doc(&st.sel);
}

void cmd_flip(int horizontal)
{
    BitMap tmp;
    Rect s = st.sel, t;
    int w = s.right - s.left, h = s.bottom - s.top;
    if (!st.hasSel) return;
    sel_change();
    bitmap_init(&tmp, w, h);
    t = tmp.bounds;
    CopyBits(&pic.bits, &tmp, &s, &t, srcCopy, 0);
    if (horizontal) {
        for (int i = 0; i < w; i++) {
            Rect a, b;
            SetRect(&a, i, 0, i + 1, h);
            SetRect(&b, s.right - 1 - i, s.top, s.right - i, s.bottom);
            CopyBits(&tmp, &pic.bits, &a, &b, srcCopy, 0);
        }
    } else {
        for (int i = 0; i < h; i++) {
            Rect a, b;
            SetRect(&a, 0, i, w, i + 1);
            SetRect(&b, s.left, s.bottom - 1 - i, s.right, s.bottom - i);
            CopyBits(&tmp, &pic.bits, &a, &b, srcCopy, 0);
        }
    }
    bitmap_free(&tmp);
    view_show_doc(&st.sel);
}
