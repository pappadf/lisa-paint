/* The document's diskfiles.
 *
 * The Desktop Manager gives each document a pathname prefix (a volume and
 * the start of a file name); a tool names its diskfiles by appending to it.
 * LisaPaint keeps two:
 *   <prefix>PIC   the picture as last saved (Save & Continue, Save & Put
 *                 Away); Revert to Previous Version goes back to it
 *   <prefix>SUS   the working state when the document is set aside or the
 *                 Lisa is turned off, so it opens again exactly as it was
 * Both hold a header (picture size, view, tool settings, selection) and the
 * picture's bits; tools/lpconv.py converts them to and from MacPaint and PNG. */
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "paint.h"

#define MAGIC "LPNT"
enum { VERSION = 2 };              /* 1: 720 x 600 pictures; 2: 720 x 800 */

typedef struct {
    char magic[4];
    int16_t version, width, height, rowBytes;
    int16_t sx, sy, zoom, tool, pattern, line, brush, hasSel;
    Rect sel;
    int16_t spare[8];
} Header;

static void path_of(char *out, const unsigned char *prefix, int suspended)
{
    memcpy(out, prefix + 1, prefix[0]);
    strcpy(out + prefix[0], suspended ? "SUS" : "PIC");
}

int doc_load(const unsigned char *prefix, int suspended)
{
    char path[300];
    Header h;
    int fd;
    long n = (long)DOC_ROWBYTES * DOC_H;
    path_of(path, prefix, suspended);
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return DOC_NONE;
    if (read(fd, &h, sizeof h) != sizeof h || memcmp(h.magic, MAGIC, 4) || h.version != VERSION ||
        h.width != DOC_W || h.height != DOC_H || h.rowBytes != pic.bits.rowBytes ||
        read(fd, pic.bits.baseAddr, n) != n) {
        close(fd);
        return DOC_BAD;
    }
    close(fd);
    st.zoom = h.zoom == ZOOM ? ZOOM : 1;
    st.sx = h.sx; st.sy = h.sy;
    st.tool = h.tool >= 0 && h.tool < T_COUNT ? h.tool : T_PENCIL;
    st.pattern = h.pattern >= 0 && h.pattern < N_PATTERNS ? h.pattern : 0;
    st.line = h.line >= 0 && h.line < N_LINES ? h.line : 1;
    st.brush = h.brush >= 0 && h.brush < N_BRUSHES ? h.brush : 1;
    st.hasSel = h.hasSel != 0;
    st.sel = h.sel;
    st.floating = 0;
    st.canUndo = 0;
    st.dirty = suspended;               /* a suspended document has unsaved edits */
    return DOC_OK;
}

int doc_save(const unsigned char *prefix, int suspended)
{
    char path[300];
    Header h;
    int fd;
    long n = (long)DOC_ROWBYTES * DOC_H;
    memset(&h, 0, sizeof h);
    memcpy(h.magic, MAGIC, 4);
    h.version = VERSION;
    h.width = DOC_W; h.height = DOC_H; h.rowBytes = pic.bits.rowBytes;
    h.sx = st.sx; h.sy = st.sy; h.zoom = st.zoom;
    h.tool = st.tool; h.pattern = st.pattern; h.line = st.line; h.brush = st.brush;
    h.hasSel = st.hasSel; h.sel = st.sel;
    path_of(path, prefix, suspended);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
        return DOC_BAD;
    if (write(fd, &h, sizeof h) != sizeof h || write(fd, pic.bits.baseAddr, n) != n) {
        close(fd);
        return DOC_BAD;
    }
    if (close(fd) != 0)
        return DOC_BAD;
    if (!suspended)
        st.dirty = 0;
    return DOC_OK;
}

void doc_remove(const unsigned char *prefix, int suspended)
{
    char path[300];
    path_of(path, prefix, suspended);
    unlink(path);
}
