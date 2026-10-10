/* The document window: tool palette (left), pattern palette (under the
 * picture), the picture view, and the Office System's own scroll bars and
 * size control, shown only while the window is active.
 *
 *   +-------+-------------------------------+--+
 *   | tools | picture view                  |  |
 *   |       |                               |V |
 *   | lines +-------------------------------+  |
 *   |       | patterns                      |  |
 *   +-------+-------------------------------+--+
 *   |                horizontal             |G |
 */
#include <string.h>
#include "paint.h"

enum {
    PAL_W = 66,                         /* tool palette: 2 + 2 cells + 1 + 2 + separator */
    CELL_W = 30, CELL_H = 20,           /* square on the screen */
    LINES_TOP = 2 + 7 * CELL_H + 8, LINE_ROW = 10,
    PAT_H = 39,                         /* pattern palette: line, 2, 2 rows, 2 */
    PAT_W = 24, PAT_CELL_H = 16,        /* pattern cells, square on the screen */
    PAT_COLS = 19,
    MIN_W = 360, MIN_H = 200,           /* smallest window */
};

WindowPtr win;
int active;
Rect viewR;
static Rect palR, patR, rH, rV;
static TSbList sbList;
static THSb hsbH, hsbV;
static int antPhase;
static long antTime;
static int cursorKind = -1;             /* -1 arrow, else tool */

/* very light gray for the area beyond the picture */
static const Pattern beyond = {{0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00}};

void view_init(void)
{
    InitWmlSb();
    InitSbList(&sbList, theHeap);
    hsbV = SbCreate(&sbList, hsbNil, tyvhV, mkpt(0, 0), 100);
    hsbH = SbCreate(&sbList, hsbNil, tyvhH, mkpt(0, 0), 100);
}

static int view_w(void) { return (viewR.right - viewR.left + st.zoom - 1) / st.zoom; }
static int view_h(void) { return (viewR.bottom - viewR.top + st.zoom - 1) / st.zoom; }
static int max_sx(void) { return imax(0, DOC_W - (viewR.right - viewR.left) / st.zoom); }
static int max_sy(void) { return imax(0, DOC_H - (viewR.bottom - viewR.top) / st.zoom); }

void view_layout(void)
{
    Rect pr;
    SetPort(win);
    pr = win->portRect;
    FlushRects(&rH, &rV);
    SetSbRect(hsbH, &rH);
    SetSbRect(hsbV, &rV);
    SetRect(&palR, pr.left, pr.top, pr.left + PAL_W, rH.top);
    SetRect(&viewR, palR.right, pr.top, rV.left, rH.top - PAT_H);
    SetRect(&patR, palR.right, viewR.bottom, rV.left, rH.top);
    st.sx = imin(st.sx, max_sx());
    st.sy = imin(st.sy, max_sy());
    view_set_thumbs();
}

void view_set_thumbs(void)
{
    int mx = max_sx(), my = max_sy();
    /* a bar shows only gray when the whole extent is visible */
    SetSbIcons(hsbH, mx ? sbAllIcons : 0);
    SetSbIcons(hsbV, my ? sbAllIcons : 0);
    SetThumb(hsbH, mx ? (int)((long)st.sx * 1000 / mx) : 0);
    SetThumb(hsbV, my ? (int)((long)st.sy * 1000 / my) : 0);
}

/* ---- coordinates -------------------------------------------------------- */

int view_pt_to_doc(Point p, Point *d)
{
    d->h = st.sx + (p.h - viewR.left) / st.zoom;
    d->v = st.sy + (p.v - viewR.top) / st.zoom;
    if (p.h < viewR.left) d->h = st.sx - 1 - (viewR.left - p.h - 1) / st.zoom;
    if (p.v < viewR.top)  d->v = st.sy - 1 - (viewR.top - p.v - 1) / st.zoom;
    return PtInRect(p, &viewR);
}

Rect view_doc_to_win(const Rect *r)
{
    Rect w;
    w.left   = viewR.left + (r->left - st.sx) * st.zoom;
    w.right  = viewR.left + (r->right - st.sx) * st.zoom;
    w.top    = viewR.top + (r->top - st.sy) * st.zoom;
    w.bottom = viewR.top + (r->bottom - st.sy) * st.zoom;
    return w;
}

/* ---- the picture ------------------------------------------------------ */

/* The selection outline is XORed onto the picture so it shows on any
 * background; clip (window coordinates) limits it to freshly copied pixels,
 * so it is never drawn twice in the same place. */
static void draw_ants_in(const Rect *clip)
{
    Rect w, c;
    if (!st.hasSel || !SectRect(clip, &viewR, &c))
        return;
    w = view_doc_to_win(&st.sel);
    SetPort(win);
    ClipRect(&c);
    PenNormal();
    PenMode(patXor);
    PenPat(&antPats[active ? antPhase : 0]);
    FrameRect(&w);
    PenNormal();
    ClipRect(&win->portRect);
}

static void draw_ants(void) { draw_ants_in(&viewR); }

static void redraw_ants(void)
{
    view_ants(1);
    draw_ants();
}

void view_show_doc(const Rect *docR)
{
    Rect s = *docR, d, docB;
    SetRect(&docB, 0, 0, DOC_W, DOC_H);
    if (!win || !SectRect(&s, &docB, &s))
        return;
    /* only the part inside the view */
    s.left = imax(s.left, st.sx);
    s.top = imax(s.top, st.sy);
    s.right = imin(s.right, st.sx + view_w());
    s.bottom = imin(s.bottom, st.sy + view_h());
    if (s.left >= s.right || s.top >= s.bottom)
        return;
    d = view_doc_to_win(&s);
    SetPort(win);
    ClipRect(&viewR);
    CopyBits(&pic.bits, &win->portBits, &s, &d, srcCopy, 0);
    ClipRect(&win->portRect);
    draw_ants_in(&d);
}

/* Erase (erase=1) or draw the selection outline. */
void view_ants(int erase)
{
    Rect e;
    int had;
    if (!st.hasSel)
        return;
    if (!erase) {
        draw_ants();
        return;
    }
    had = st.hasSel;
    st.hasSel = 0;                      /* view_show_doc must not redraw them */
    e = st.sel; e.bottom = e.top + 1;     view_show_doc(&e);
    e = st.sel; e.top = e.bottom - 1;     view_show_doc(&e);
    e = st.sel; e.right = e.left + 1;     view_show_doc(&e);
    e = st.sel; e.left = e.right - 1;     view_show_doc(&e);
    st.hasSel = had;
}

static void draw_view(void)
{
    Rect all, w;
    SetPort(win);
    /* the area beyond the picture, if any */
    SetRect(&all, 0, 0, DOC_W, DOC_H);
    w = view_doc_to_win(&all);
    if (w.right < viewR.right || w.bottom < viewR.bottom) {
        ClipRect(&viewR);
        FillRect(&viewR, &beyond);
        ClipRect(&win->portRect);
    }
    SetRect(&all, st.sx, st.sy, st.sx + view_w(), st.sy + view_h());
    view_show_doc(&all);
}

/* ---- palettes ------------------------------------------------------------ */

static Rect tool_cell(int t)
{
    Rect r;
    int left = palR.left + 2 + (t % 2) * CELL_W, top = palR.top + 2 + (t / 2) * CELL_H;
    SetRect(&r, left, top, left + CELL_W + 1, top + CELL_H + 1);
    return r;
}

static Rect line_cell(int i)
{
    Rect r;
    SetRect(&r, palR.left + 4, palR.top + LINES_TOP + i * LINE_ROW,
            palR.right - 6, palR.top + LINES_TOP + (i + 1) * LINE_ROW);
    return r;
}

static Rect pat_cell(int k)
{
    Rect r;
    int left = patR.left + 52 + (k % PAT_COLS) * (PAT_W + 1);
    int top = patR.top + 3 + (k / PAT_COLS) * (PAT_CELL_H + 1);
    SetRect(&r, left, top, left + PAT_W + 1, top + PAT_CELL_H + 1);
    return r;
}

static Rect pat_swatch(void)
{
    Rect r;
    SetRect(&r, patR.left + 6, patR.top + 3, patR.left + 6 + 40, patR.top + 3 + 2 * PAT_CELL_H + 1);
    return r;
}

void view_draw_palettes(void)
{
    Rect r;
    SetPort(win);
    ClipRect(&win->portRect);
    PenNormal();
    /* tools */
    EraseRect(&palR);
    for (int t = 0; t < T_COUNT; t++) {
        r = tool_cell(t);
        FrameRect(&r);
        draw_tool_icon(t, &r);
        if (t == st.tool) {
            InsetRect(&r, 1, 1);
            InvertRect(&r);
        }
    }
    /* line widths */
    for (int i = 0; i < N_LINES; i++) {
        Rect c = line_cell(i), l = c;
        int mid = (c.top + c.bottom) / 2;
        l.left += 6; l.right -= 4;
        l.top = mid - lineSizes[i].v / 2;
        l.bottom = l.top + lineSizes[i].v;
        PaintRect(&l);
        if (i == st.line) {             /* a pointer at the left */
            MoveTo(c.left, mid - 2); LineTo(c.left + 2, mid);
            LineTo(c.left, mid + 2); LineTo(c.left, mid - 2);
        }
    }
    MoveTo(palR.right - 1, palR.top);
    LineTo(palR.right - 1, palR.bottom - 1);

    /* patterns */
    EraseRect(&patR);
    MoveTo(patR.left, patR.top);
    LineTo(patR.right - 1, patR.top);
    r = pat_swatch();
    FillRect(&r, &patterns[st.pattern]);
    FrameRect(&r);
    for (int k = 0; k < N_PATTERNS; k++) {
        Rect c = pat_cell(k);
        if (c.right > patR.right)
            continue;
        FillRect(&c, &patterns[k]);
        FrameRect(&c);
        if (k == st.pattern) {
            Rect o = c;
            InsetRect(&o, -1, -1);
            PenSize(2, 1);
            FrameRect(&o);
            PenNormal();
        }
    }
}

void view_draw_all(void)
{
    if (!win)
        return;
    SetPort(win);
    ClipRect(&win->portRect);
    view_draw_palettes();
    draw_view();
    if (active) {
        PaintSbar(hsbH);
        PaintSbar(hsbV);
        PaintGrow();
    }
}

void view_activate(int on)
{
    active = on;
    if (!win)
        return;
    SetPort(win);
    ClipRect(&win->portRect);
    if (on) {
        view_layout();
        PaintSbar(hsbH);
        PaintSbar(hsbV);
        PaintGrow();
    } else {
        HideSBorder(&win->portRect);
        SetCursor(&arrow);
        cursorKind = -1;
    }
    redraw_ants();                      /* animated when active, still when not */
}

/* ---- scrolling ------------------------------------------------------------ */

void view_scroll_to(int sx, int sy)
{
    sx = imax(0, imin(sx, max_sx()));
    sy = imax(0, imin(sy, max_sy()));
    if (sx == st.sx && sy == st.sy)
        return;
    st.sx = sx;
    st.sy = sy;
    view_set_thumbs();
    SetPort(win);
    PaintSbar(hsbH);
    PaintSbar(hsbV);
    draw_view();
}

static void pause_hundredths(long n)
{
    long t = Time() + n;
    while (Time() < t && StillDown())
        ;
}

static void scroll_bar_hit(THSb hsb, TIcon icon, Point where)
{
    int vert = hsb == hsbV;
    int page = vert ? view_h() - 16 / st.zoom : view_w() - 24 / st.zoom;
    int step = vert ? 16 / st.zoom : 24 / st.zoom;
    int dir;
    if (step < 1) step = 1;
    switch (icon) {
    case iconArwA:
    case iconArwB:                      /* continuous, while the button is down */
        dir = icon == iconArwA ? -1 : 1;
        PaintArw(hsb, icon, true);
        do {
            if (vert) view_scroll_to(st.sx, st.sy + dir * step);
            else      view_scroll_to(st.sx + dir * step, st.sy);
            PaintArw(hsb, icon, true);
            pause_hundredths(3);
        } while (StillDown());
        PaintArw(hsb, icon, false);
        break;
    case iconPagA: case iconGryA:
    case iconPagB: case iconGryB:       /* a page at a time */
        dir = (icon == iconPagA || icon == iconGryA) ? -1 : 1;
        do {
            if (vert) view_scroll_to(st.sx, st.sy + dir * page);
            else      view_scroll_to(st.sx + dir * page, st.sy);
            pause_hundredths(25);
        } while (StillDown());
        break;
    case iconThumb: {                   /* drag the elevator; jump on release */
        Integer c = 0;
        DragThumb(hsb, where, &c);
        SetThumb(hsb, c);
        if (vert) view_scroll_to(st.sx, (int)((long)c * max_sy() / 1000));
        else      view_scroll_to((int)((long)c * max_sx() / 1000), st.sy);
        view_set_thumbs();
        PaintSbar(hsb);
        break;
    }
    default:
        break;
    }
}

void view_grow(Point where)
{
    Point br;
    DragFrame(where, true, &br);
    br.h = imax(br.h - win->portRect.left, MIN_W);
    br.v = imax(br.v - win->portRect.top, MIN_H);
    SetPort(win);
    HideSBorder(&win->portRect);
    FolderSize(win, br.h, br.v, true);
    view_layout();
    InvalRect(&win->portRect);
}

/* Magnify or show actual size, keeping the middle of the view in place. */
void view_set_zoom(int z)
{
    int cx = st.sx + view_w() / 2, cy = st.sy + view_h() / 2;
    st.zoom = z;
    view_layout();
    st.sx = imax(0, imin(cx - view_w() / 2, max_sx()));
    st.sy = imax(0, imin(cy - view_h() / 2, max_sy()));
    view_set_thumbs();
    SetPort(win);
    draw_view();
    if (active) {
        PaintSbar(hsbH);
        PaintSbar(hsbV);
    }
}

/* ---- clicks ----------------------------------------------------------------- */

void view_click(Point where, const EventRecord *ev)
{
    THSb hsb;
    TIcon icon;
    Point d;
    SetPort(win);
    if (active && FGrowHit(where)) {
        view_grow(where);
        return;
    }
    if (active && FSbHit(&sbList, where, &hsb, &icon)) {
        scroll_bar_hit(hsb, icon, where);
        return;
    }
    if (PtInRect(where, &viewR)) {
        view_pt_to_doc(where, &d);
        tool_track(d, ev);
        menus_update();
        return;
    }
    if (PtInRect(where, &palR)) {
        for (int t = 0; t < T_COUNT; t++) {
            Rect c = tool_cell(t);
            if (PtInRect(where, &c)) {
                if (t != T_SELECT)
                    drop_selection();
                st.tool = t;
                view_draw_palettes();
                return;
            }
        }
        for (int i = 0; i < N_LINES; i++) {
            Rect c = line_cell(i);
            if (PtInRect(where, &c)) {
                st.line = i;
                view_draw_palettes();
                menus_update();
                return;
            }
        }
        return;
    }
    if (PtInRect(where, &patR)) {
        for (int k = 0; k < N_PATTERNS; k++) {
            Rect c = pat_cell(k);
            if (PtInRect(where, &c) && c.right <= patR.right) {
                st.pattern = k;
                view_draw_palettes();
                return;
            }
        }
    }
}

/* Called while idle and active: cursor shape and marching ants. */
void view_idle(void)
{
    Point p;
    int kind;
    if (!win || !active)
        return;
    SetPort(win);
    GetMouse(&p);
    kind = PtInRect(p, &viewR) ? st.tool : -1;
    if (kind != cursorKind) {
        cursorKind = kind;
        if (kind < 0) SetCursor(&arrow);
        else set_tool_cursor(kind);
    }
    if (st.hasSel && Time() - antTime >= 12) {
        antTime = Time();
        antPhase = (antPhase + 1) & 7;
        redraw_ants();
    }
}
