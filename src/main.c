/* LisaPaint: startup, the event loop, menus, and the Desktop Manager's
 * document protocol (lisa/filer.h).
 *
 * Like every Office System tool, LisaPaint is document-centric: the user
 * tears a sheet off the "LisaPaint Paper" pad and opens it, and the Desktop
 * Manager starts LisaPaint and hands it the document's window.  There is no
 * New, Open, Save As or Quit: the File/Print menu offers Set Aside, Save &
 * Put Away, Save & Continue and Revert to Previous Version. */
#include <string.h>
#include "lisa.h"
#include "lisa_os.h"
#include "paint.h"

/* menus, as in lisapaint.phrase */
enum { M_FILE = 1, M_EDIT, M_SEL, M_LINES, M_BRUSH, M_LAYOUT, N_MENUS = 6 };
enum { F_SETASIDEALL = 1, F_SETASIDE, F_PUTAWAY = 4, F_SAVE, F_REVERT,
       F_FORMAT = 8, F_PRINT, F_MONITOR };
enum { E_UNDO = 1, E_CUT = 3, E_COPY, E_PASTE, E_CLEAR, E_SELALL = 8 };
enum { S_INVERT = 1, S_FILL, S_FLIPH = 4, S_FLIPV };
enum { L_ACTUAL = 1, L_MAGNIFY };

static char heap[32768];                /* QuickDraw / Window Manager heap */
static MenuInfo menus[N_MENUS];
PAlertRec alertFile;
static unsigned char docPrefix[256];    /* the document's diskfile prefix */
static int hasDoc;                      /* docPrefix is valid */
static int pendingAlert;                /* to show once the window is active */

static pascal Integer HeapFull(void *zone, Integer bytesNeeded)
{
    (void)zone; (void)bytesNeeded;
    return 0;
}

static pascal void FailedAlert(void)
{
}

/* SYS_TERMINATE exception handler: the protocol requires ImDying first. */
static pascal void LastWishes(void *environment, void *data)
{
    (void)environment; (void)data;
    ImDying();
}

/* "-volume-{Tn}": this tool's file prefix, from its program pathname
 * ("-volume-{Tn}obj"). */
static void tool_prefix(unsigned char prefix[64])
{
    static unsigned char info[600];     /* procinfoRec: progpathname first */
    Integer err;
    int n;
    lisa_info_process(&err, lisa_my_id(), info);
    n = info[0] >= 3 ? info[0] - 3 : 0;
    if (n > 62) n = 62;
    memcpy(prefix + 1, info + 1, n);
    prefix[0] = (unsigned char)n;
}

static MenuInfo *menu(int id) { return &menus[id - 1]; }

static void enable(int id, int item, int on)
{
    if (on) EnableItem(menu(id), item);
    else    DisableItem(menu(id), item);
}

/* Bring the menus in line with the document's state: commands that would do
 * nothing are dimmed, the current line width and brush are checked, and
 * Set Aside names the document. */
void menus_update(void)
{
    unsigned char title[256], item[80];
    if (!win)
        return;
    enable(M_FILE, F_SAVE, hasDoc);
    enable(M_FILE, F_REVERT, hasDoc);
    enable(M_FILE, F_FORMAT, 0);
    enable(M_FILE, F_PRINT, 0);
    enable(M_FILE, F_MONITOR, 0);
    GetFldrTitle(win, title);
    if (title[0] && title[0] < 60) {
        item[0] = (unsigned char)(12 + title[0]);
        memcpy(item + 1, "Set Aside \"", 11);
        memcpy(item + 12, title + 1, title[0]);
        item[12 + title[0]] = '"';
        SetItem(menu(M_FILE), F_SETASIDE, item);
    }
    enable(M_EDIT, E_UNDO, st.canUndo);
    enable(M_EDIT, E_CUT, st.hasSel);
    enable(M_EDIT, E_COPY, st.hasSel);
    enable(M_EDIT, E_PASTE, !clip_empty());
    enable(M_EDIT, E_CLEAR, st.hasSel);
    for (int i = 1; i <= 5; i++)
        if (i != 3)
            enable(M_SEL, i, st.hasSel);
    for (int i = 0; i < N_LINES; i++)
        CheckItem(menu(M_LINES), i + 1, i == st.line);
    for (int i = 0; i < N_BRUSHES; i++)
        CheckItem(menu(M_BRUSH), i + 1, i == st.brush);
    /* a pair of items, the one in effect dimmed */
    enable(M_LAYOUT, L_ACTUAL, st.zoom != 1);
    enable(M_LAYOUT, L_MAGNIFY, st.zoom == 1);
}

static void activate(void)
{
    for (int i = 0; i < N_MENUS; i++)
        InsertMenu(&menus[i], 0);
    menus_update();
    DrawMenuBar();
    view_activate(1);
    if (pendingAlert) {
        StopAlert(alertFile, pendingAlert);
        pendingAlert = 0;
    }
}

/* ---- documents ------------------------------------------------------------ */

static void close_doc(void)
{
    commit_selection();
    st.hasSel = 0;
    win = 0;
    active = 0;
    hasDoc = 0;
}

/* Make the edits permanent: write <prefix>PIC, forget the suspended state. */
static int save_doc(const unsigned char *prefix)
{
    commit_selection();
    if (doc_save(prefix, 0) != DOC_OK)
        return 0;
    if (prefix == docPrefix)
        doc_remove(docPrefix, 1);
    return 1;
}

static void open_doc(const EventRecord *ev, const FilerExt *fx)
{
    Integer err;
    if (win && ev->who != win) {        /* one picture at a time */
        TellFiler(&err, docClosd, noMoreDocs, ev->who);
        return;
    }
    win = ev->who;
    TakeWindow(win);
    memset(&st, 0, sizeof st);
    st.zoom = 1; st.line = 1; st.brush = 1; st.tool = T_PENCIL;
    pic_clear();
    hasDoc = fx->theFlrOp == fcResume && fx->thePrefix[0];
    if (hasDoc) {
        memcpy(docPrefix, fx->thePrefix, fx->thePrefix[0] + 1);
        int r = doc_load(docPrefix, 1);          /* set aside: working state */
        if (r == DOC_NONE)
            r = doc_load(docPrefix, 0);          /* else the saved picture */
        if (r == DOC_BAD) {
            pic_clear();
            pendingAlert = A_CANTREAD;
        }
    }
    view_layout();
    view_draw_all();
    if (win == activeFolder)
        activate();
}

static void filer_event(EventRecord *ev, int *done)
{
    FilerExt fx;
    Integer err;
    int ok;
    GetAddParms(&err, ev, &fx);
    switch (fx.theFlrOp) {
    case fcResume:
    case fcNone:
        open_doc(ev, &fx);
        break;
    case fcClose:                       /* put away: make the edits permanent */
        ok = !hasDoc || save_doc(docPrefix);
        TellFiler(&err, ok ? docClosd : docNotClosed, ok ? allOk : cantWrite, ev->who);
        if (ok) close_doc();
        break;
    case fcSuspend:                     /* set aside, or the Lisa is turning off */
        commit_selection();
        ok = !hasDoc || doc_save(docPrefix, 1) == DOC_OK;
        TellFiler(&err, ok ? docClosd : docNotClosed, ok ? allOk : cantWrite, ev->who);
        if (ok) close_doc();
        break;
    case fcPut:                         /* to new diskfiles; the old ones go */
        ok = save_doc(fx.thePrefix);
        if (ok && hasDoc) {
            doc_remove(docPrefix, 0);
            doc_remove(docPrefix, 1);
        }
        TellFiler(&err, ok ? docXfered : docNotXfered, ok ? allOk : cantWrite, ev->who);
        if (ok) close_doc();
        break;
    case fcCopy:                        /* a copy in new diskfiles; stay open */
        ok = save_doc(fx.thePrefix);
        TellFiler(&err, ok ? docXfered : docNotXfered, ok ? allOk : cantWrite, ev->who);
        break;
    case fcShred:
        TellFiler(&err, docClosd, allOk, ev->who);
        close_doc();
        break;
    case fcTerminate:
        *done = 1;
        break;
    default:
        TellFiler(&err, docClosd, internalError, ev->who);
        break;
    }
}

/* ---- commands ---------------------------------------------------------------- */

static void command(int m, int i)
{
    Integer err;
    switch (m) {
    case M_FILE:
        switch (i) {
        case F_SETASIDEALL: DoFilingCmd(cmdCloseAll); break;
        case F_SETASIDE:    DoFilingCmd(cmdClose); break;
        case F_PUTAWAY:
            if (hasDoc && !save_doc(docPrefix)) {
                StopAlert(alertFile, A_CANTSAVE);
                break;
            }
            view_activate(0);
            TellFiler(&err, docClosd, docPutBack, win);
            close_doc();
            return;
        case F_SAVE:
            if (!save_doc(docPrefix))
                StopAlert(alertFile, A_CANTSAVE);
            break;
        case F_REVERT:
            if (CautionAlert(alertFile, A_REVERT)) {
                drop_selection();
                if (doc_load(docPrefix, 0) != DOC_OK)
                    pic_clear();
                doc_remove(docPrefix, 1);
                st.dirty = 0;
                view_layout();
                view_draw_all();
            }
            break;
        }
        break;
    case M_EDIT:
        switch (i) {
        case E_UNDO:   cmd_undo(); break;
        case E_CUT:    cmd_cut(); break;
        case E_COPY:   cmd_copy(); break;
        case E_PASTE:  cmd_paste(); break;
        case E_CLEAR:  cmd_clear(); break;
        case E_SELALL: cmd_select_all(); break;
        }
        break;
    case M_SEL:
        switch (i) {
        case S_INVERT: cmd_invert(); break;
        case S_FILL:   cmd_fill(); break;
        case S_FLIPH:  cmd_flip(1); break;
        case S_FLIPV:  cmd_flip(0); break;
        }
        break;
    case M_LINES:
        st.line = i - 1;
        view_draw_palettes();
        break;
    case M_BRUSH:
        st.brush = i - 1;
        if (st.tool != T_BRUSH) {
            drop_selection();
            st.tool = T_BRUSH;
        }
        view_draw_palettes();
        break;
    case M_LAYOUT:
        view_set_zoom(i == L_MAGNIFY ? ZOOM : 1);
        break;
    }
    if (win)
        menus_update();
}

static void key_down(const EventRecord *ev)
{
    Integer m = 0, i = 0;
    if (ev->appleKey) {                 /* Apple-key equivalents of menu items */
        MenuKey(ev->ascii, &m, &i);
        if (m) {
            HiLiteMenu(m);
            command(m, i);
            HiLiteMenu(0);
        }
        return;
    }
    if (ev->ascii == 8 && st.hasSel)     /* Backspace clears the selection */
        command(M_EDIT, E_CLEAR);
}

int main(void)
{
    static const unsigned char excep_name[] = "\015SYS_TERMINATE";
    unsigned char name[18], prefix[64];
    EventRecord ev;
    Integer err;
    int done = 0;

    InitGraf(&thePort);
    OpenWM();
    memcpy(name, excep_name, sizeof excep_name);
    lisa_declare_excep_hdl(&err, name, (void *)LastWishes);
    SetPort(deskPort);
    InitHeap(heap, heap + sizeof heap, (void *)HeapFull);
    tool_prefix(prefix);
    InitMP(theHeap, prefix, (void *)FailedAlert, 2, menus, &alertFile, &err);
    if (err > 0) {
        TellFiler(&err, initFailed, internalError, 0);
        ImDying();
        return 1;
    }
    InitWmlCrs(&err);
    art_init();
    tools_init();
    view_init();

    while (!done) {
        /* Block in GetEvent while inactive; while active, poll so that the
         * cursor and the marching ants stay alive, and let others run. */
        if (ImActive() && !EventAvail()) {
            view_idle();
            LetOthersRun();
            continue;
        }
        GetEvent(&ev);
        switch (ev.what) {
        case filerEvent:
            filer_event(&ev, &done);
            break;
        case folderActivate:
            TakeControl(&ev, false, false);
            if (win && ev.who == win)
                activate();
            break;
        case folderDeactivate:
            if (win && ev.who == win)
                view_activate(0);
            GiveControl(&ev);
            break;
        case folderUpdate:
            BeginUpdate(ev.who);
            if (win && ev.who == win)
                view_draw_all();
            EndUpdate(ev.who);
            break;
        case buttonDown:
            if (ev.who == menuFolder) {
                Integer m, i;
                MenuSelect(ev.where, &m, &i);
                command(m, i);
                HiLiteMenu(0);
            } else if (win && ev.who == win) {
                view_click(ev.where, &ev);
            }
            break;
        case keyDown:
            if (win)
                key_down(&ev);
            break;
        }
    }
    ImDying();
    return 0;
}
