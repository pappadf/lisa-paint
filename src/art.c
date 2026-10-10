/* Icons, patterns and cursors.
 *
 * Lisa pixels are about 1.5 times taller than wide (90 x 60 dots per inch),
 * so the art is drawn for that: a palette cell of 30 x 20 pixels is square
 * on the screen, a circle is 3 wide for every 2 tall. */
#include <string.h>
#include "paint.h"

/* ---- patterns ------------------------------------------------------------ */
const Pattern patterns[N_PATTERNS] = {
    {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},   /* black */
    {{0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD}},   /* dark gray */
    {{0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55}},   /* gray */
    {{0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22}},   /* light gray */
    {{0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00}},   /* lighter */
    {{0x80, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00}},   /* sparse */
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},   /* white */
    {{0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00}},   /* horizontal lines */
    {{0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00}},   /* wide horizontal */
    {{0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88}},   /* vertical lines */
    {{0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC}},   /* wide vertical */
    {{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}},   /* diagonal / */
    {{0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}},   /* diagonal \ */
    {{0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x81}},   /* thick / */
    {{0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x81}},   /* thick \ */
    {{0xFF, 0x88, 0x88, 0x88, 0xFF, 0x88, 0x88, 0x88}},   /* grid */
    {{0xFF, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}},   /* large grid */
    {{0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00}},   /* dot grid */
    {{0xFF, 0x80, 0x80, 0x80, 0xFF, 0x08, 0x08, 0x08}},   /* bricks */
    {{0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81}},   /* crosshatch x */
    {{0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88}},   /* fine diagonal */
    {{0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F}},   /* checkerboard */
    {{0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33}},   /* small checks */
    {{0x00, 0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0x18, 0x00}},   /* big dots */
    {{0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x81, 0x81}},   /* dots */
    {{0x10, 0x28, 0x44, 0x82, 0x44, 0x28, 0x10, 0x00}},   /* diamonds */
    {{0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x10, 0x00}},   /* solid diamonds */
    {{0x80, 0x80, 0x41, 0x3E, 0x08, 0x08, 0x14, 0xE3}},   /* scales */
    {{0x88, 0x14, 0x22, 0x41, 0x88, 0x00, 0xAA, 0x00}},   /* weave */
    {{0x08, 0x1C, 0x22, 0xC1, 0x80, 0x01, 0x02, 0x04}},   /* waves */
    {{0xF8, 0x74, 0x22, 0x47, 0x8F, 0x17, 0x22, 0x71}},   /* basket */
    {{0x00, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x00}},   /* boxes */
    {{0xFF, 0x81, 0xBD, 0xA5, 0xA5, 0xBD, 0x81, 0xFF}},   /* nested boxes */
    {{0x01, 0x01, 0x01, 0xFF, 0x10, 0x10, 0x10, 0xFF}},   /* offset bricks */
    {{0xE0, 0xD1, 0xBB, 0x1D, 0x0E, 0x17, 0xBB, 0x71}},   /* zigzag weave */
    {{0x82, 0x44, 0x39, 0x44, 0x82, 0x01, 0x01, 0x01}},   /* stars */
    {{0x55, 0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55, 0xFF}},   /* very dark */
    {{0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF}},   /* sparse lines */
};

/* Pen sizes (h, v) of the line widths: each is about as tall as it is wide
 * on the screen, i.e. 3 pixels wide for every 2 tall. */
const Point lineSizes[N_LINES] = {
    {.v = 1, .h = 1}, {.v = 2, .h = 3}, {.v = 3, .h = 5}, {.v = 4, .h = 6}, {.v = 6, .h = 9},
};

/* Marching ants: diagonal stripes, one per animation phase. */
const Pattern antPats[8] = {
    {{0xF0, 0xE1, 0xC3, 0x87, 0x0F, 0x1E, 0x3C, 0x78}},
    {{0xE1, 0xC3, 0x87, 0x0F, 0x1E, 0x3C, 0x78, 0xF0}},
    {{0xC3, 0x87, 0x0F, 0x1E, 0x3C, 0x78, 0xF0, 0xE1}},
    {{0x87, 0x0F, 0x1E, 0x3C, 0x78, 0xF0, 0xE1, 0xC3}},
    {{0x0F, 0x1E, 0x3C, 0x78, 0xF0, 0xE1, 0xC3, 0x87}},
    {{0x1E, 0x3C, 0x78, 0xF0, 0xE1, 0xC3, 0x87, 0x0F}},
    {{0x3C, 0x78, 0xF0, 0xE1, 0xC3, 0x87, 0x0F, 0x1E}},
    {{0x78, 0xF0, 0xE1, 0xC3, 0x87, 0x0F, 0x1E, 0x3C}},
};

/* ---- tool icons (24 x 14, drawn into 30 x 20 cells) --------------------- */
enum { ICON_W = 24, ICON_H = 14, ICON_ROWBYTES = 4 };

static const char *const iconArt[T_COUNT][ICON_H] = {
    [T_BUCKET] = {
        "          ##            ",
        "         #  #           ",
        "        #    ####       ",
        "       #  ##   # #      ",
        "      #  #  #   #  #    ",
        "     #   #  #  ##  ##   ",
        "    #     ##   ## ###   ",
        "     #        #   ###   ",
        "      #      #    ###   ",
        "       #    #     ###   ",
        "        #  #       #    ",
        "         ##        #    ",
        "                        ",
        "                        "},
    [T_SPRAY] = {
        "  #  #  #     ####      ",
        "#  #   #      #  #      ",
        "  #  #  #   ########    ",
        "#   #  #  # #      #    ",
        "  #  #  #   #      #    ",
        "#  #   #    # #### #    ",
        "  #   #     # #  # #    ",
        "            # #### #    ",
        "            #      #    ",
        "            #      #    ",
        "            #      #    ",
        "            ########    ",
        "                        ",
        "                        "},
    [T_BRUSH] = {
        "                    ### ",
        "                   #  # ",
        "                  #  #  ",
        "                 #  #   ",
        "                #  #    ",
        "               #  #     ",
        "             #####      ",
        "            #   #       ",
        "          ##   #        ",
        "        ### # #         ",
        "      ## # ###          ",
        "    ## # ##             ",
        "  ######                ",
        "                        "},
    [T_PENCIL] = {
        "                  ##    ",
        "                ##  #   ",
        "               #  ## #  ",
        "             ##  #  #   ",
        "           ##  ##  #    ",
        "         ##  ##  ##     ",
        "       ##  ##  ##       ",
        "     ##  ##  ##         ",
        "    #  ##  ##           ",
        "   #  #  ##             ",
        "  #   ###               ",
        "  # ##                  ",
        "  ##                    ",
        "                        "},
    [T_ERASER] = {
        "                        ",
        "         ############   ",
        "        #          ##   ",
        "       #          # #   ",
        "      #          #  #   ",
        "     #          #   #   ",
        "    ############    #   ",
        "    #          #   #    ",
        "    #          #  #     ",
        "    #          # #      ",
        "    #          ##       ",
        "    ############        ",
        "                        ",
        "                        "},
};

static uint8_t iconBits[T_COUNT][ICON_H * ICON_ROWBYTES];

static void make_icon(int t)
{
    for (int y = 0; y < ICON_H; y++)
        for (int x = 0; x < ICON_W; x++)
            if (iconArt[t][y][x] == '#')
                iconBits[t][y * ICON_ROWBYTES + (x >> 3)] |= 0x80 >> (x & 7);
}

/* The shape tools are drawn with QuickDraw, the others from iconArt. */
void draw_tool_icon(int tool, const Rect *cell)
{
    Rect r = *cell;
    InsetRect(&r, 5, 4);                /* 20 x 12 shape area */
    PenNormal();
    switch (tool) {
    case T_SELECT:
        PenPat(&antPats[0]);
        FrameRect(&r);
        PenNormal();
        break;
    case T_LINE:
        MoveTo(r.left, r.bottom - 1);
        LineTo(r.right - 1, r.top);
        break;
    case T_RECT:   FrameRect(&r); break;
    case T_FRECT:  FillRect(&r, &patterns[2]); FrameRect(&r); break;
    case T_RRECT:  FrameRoundRect(&r, 9, 6); break;
    case T_FRRECT: FillRoundRect(&r, 9, 6, &patterns[2]); FrameRoundRect(&r, 9, 6); break;
    case T_OVAL:   FrameOval(&r); break;
    case T_FOVAL:  FillOval(&r, &patterns[2]); FrameOval(&r); break;
    default: {
        BitMap bm;
        Rect src, dst;
        bm.baseAddr = (QDPtr)iconBits[tool];
        bm.rowBytes = ICON_ROWBYTES;
        SetRect(&bm.bounds, 0, 0, ICON_W, ICON_H);
        src = bm.bounds;
        SetRect(&dst, cell->left + 3, cell->top + 3, cell->left + 3 + ICON_W, cell->top + 3 + ICON_H);
        CopyBits(&bm, &thePort->portBits, &src, &dst, srcOr, 0);
        break;
    }
    }
}

/* ---- cursors (16 x 16) ----------------------------------------------------- */
typedef struct { const char *art[16]; int h, v; } CursorArt;

static const CursorArt crossArt = {{
    "       #        ",
    "       #        ",
    "       #        ",
    "       #        ",
    "       #        ",
    "                ",
    "                ",
    "#######   ######",
    "                ",
    "                ",
    "       #        ",
    "       #        ",
    "       #        ",
    "       #        ",
    "       #        ",
    "                "}, 7, 7};

static const CursorArt pencilArt = {{
    "            ##  ",
    "           #  # ",
    "          #  ## ",
    "         #  # # ",
    "        #  # #  ",
    "       #  # #   ",
    "      #  # #    ",
    "     #  # #     ",
    "    #  # #      ",
    "   #  # #       ",
    "  ##### #       ",
    "  #   ##        ",
    " ##  ##         ",
    " # ###          ",
    " ###            ",
    " #              "}, 1, 15};

static const CursorArt bucketArt = {{
    "     ##         ",
    "    #  #        ",
    "   #    ###     ",
    "  #  ##   ##    ",
    " #  #  #  # #   ",
    "#   #  # ## ##  ",
    " #   ##  ## ### ",
    "  #     #   ### ",
    "   #   #    ### ",
    "    # #     ### ",
    "     #       #  ",
    "             #  ",
    "             #  ",
    "                ",
    "                ",
    "                "}, 13, 12};

static const CursorArt sprayArt = {{
    "# # #    ####   ",
    " # # #   #  #   ",
    "# # #  ######## ",
    " # # # #      # ",
    "# # #  # #### # ",
    "       # #  # # ",
    "       # #### # ",
    "       #      # ",
    "       #      # ",
    "       #      # ",
    "       ######## ",
    "                ",
    "                ",
    "                ",
    "                ",
    "                "}, 2, 2};

static const CursorArt eraserArt = {{   /* 16 x 11: the eraser's own size */
    "################",
    "#              #",
    "#              #",
    "#              #",
    "#              #",
    "#              #",
    "#              #",
    "#              #",
    "#              #",
    "#              #",
    "################",
    "                ",
    "                ",
    "                ",
    "                ",
    "                "}, 8, 5};

static Cursor crossCrs, pencilCrs, bucketCrs, sprayCrs, eraserCrs;

/* Data from the art; the mask is the data grown by one pixel, a white
 * outline that keeps the cursor visible on black. */
static void make_cursor(const CursorArt *a, Cursor *c, int outline)
{
    memset(c, 0, sizeof *c);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
            if (a->art[y][x] == '#') {
                c->data[y] |= (Integer)(0x8000u >> x);
                for (int dy = -outline; dy <= outline; dy++)
                    for (int dx = -outline; dx <= outline; dx++)
                        if (y + dy >= 0 && y + dy < 16 && x + dx >= 0 && x + dx < 16)
                            c->mask[y + dy] |= (Integer)(0x8000u >> (x + dx));
            }
    c->hotSpot.h = a->h;
    c->hotSpot.v = a->v;
}

void set_tool_cursor(int tool)
{
    switch (tool) {
    case T_PENCIL: SetCursor(&pencilCrs); break;
    case T_BUCKET: SetCursor(&bucketCrs); break;
    case T_SPRAY:  SetCursor(&sprayCrs); break;
    case T_ERASER: SetCursor(&eraserCrs); break;
    default:       SetCursor(&crossCrs); break;
    }
}

void art_init(void)
{
    for (int t = 0; t < T_COUNT; t++)
        if (iconArt[t][0])
            make_icon(t);
    make_cursor(&crossArt, &crossCrs, 1);
    make_cursor(&pencilArt, &pencilCrs, 1);
    make_cursor(&bucketArt, &bucketCrs, 1);
    make_cursor(&sprayArt, &sprayCrs, 1);
    make_cursor(&eraserArt, &eraserCrs, 0);
}
