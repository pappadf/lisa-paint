# LisaPaint

A pixel painting tool for the Apple Lisa Office System (LOS 3.x), in the
spirit of MacPaint but built the Lisa way. It's written in C and cross-compiled
with [lisa-c](https://github.com/pappadf/lisa-c); it runs on the LOS desktop
(tested in the granny-smith emulator).

## What it does

- **Tools** (palette on the left): selection, paint bucket, spray can, brush,
  pencil, eraser, line, and rectangle, rounded rectangle and oval, each hollow
  or filled.
- **38 patterns** (palette under the picture), **5 line widths** (palette, or
  the Lines menu), **7 brush shapes** (Brush menu).
- **Selection**: drag a rectangle (marching ants), drag it to move it, Cut /
  Copy / Paste / Clear, Select All of Document; Invert, Fill with Pattern,
  Flip Horizontal, Flip Vertical.
- **Undo Last Change**: one level. Choosing Undo again redoes the change.
- **Magnify Pixels** / **Show Actual Size** (Page Layout menu): a 4x view in
  which every tool works.
- **Shift** constrains lines to 0, 45 and 90 degrees, and rectangles and ovals
  to squares and circles, *as they look on the Lisa screen* (see below).
- **Scroll bars and size control**: the Office System's own, shown only while
  the window is active (arrows, page-at-a-time gray areas, draggable
  elevator). There is no "hand" tool; use the scroll bars.
- **Documents**: the picture is an 8 x 10 inch page (720 x 600 Lisa pixels).

## Lisa conventions it follows

- It's document-centric. The user tears a sheet off the "LisaPaint Paper" pad
  and opens it, and the Desktop Manager starts LisaPaint. There is no New,
  Open, Save As, Close or Quit.
- **File/Print**: Set Aside Everything, Set Aside "*document*", Save & Put
  Away, Save & Continue, Revert to Previous Version, and the printing items
  (dimmed: printing is not implemented).
- **Edit**: Undo Last Change, Cut (Apple-X), Copy (Apple-C), Paste (Apple-V),
  Clear, Select All of Document (Apple-A).
- Unavailable commands are dimmed. Show Actual Size and Magnify Pixels are a
  pair, with the one in effect dimmed.
- Alerts live in the phrase file, as full sentences. Revert to Previous Version
  asks first, in a Caution alert: "... Once you click OK, you will not be able
  to change your mind, even with Undo."
- **Non-square pixels.** Lisa pixels are about 1.5 times taller than wide (90 x
  60 dots per inch). The palette cells (30 x 20), the round brushes, the eraser
  (16 x 11), the cursors and the Shift constraints are all drawn for that: a
  circle is 3 pixels wide for every 2 tall. Magnify Pixels scales both
  directions by 4, so the picture keeps its proportions.

## Documents on disk

The Desktop Manager gives each document a diskfile prefix (for example
`-#13-{D103T21}`). LisaPaint keeps two files:

- `<prefix>PIC`: the picture as last saved. Save & Continue, Save & Put Away
  and closing write it, and Revert to Previous Version reads it back.
- `<prefix>SUS`: the working state when the document is suspended (the
  Desktop Manager's fcSuspend). It's read first when the document is opened.

Each file is a small header (picture size, view position, tool, pattern, line
width, brush, selection) followed by the 54,000 bytes of the picture. On LOS 3.1,
Set Aside only hides the window: LisaPaint keeps the picture in memory until the
document is put away.

## Building and running

lisa-c comes in as a git submodule (`lisa-c/`, pinned to the version
LisaPaint is tested with):

```bash
git clone --recursive https://github.com/pappadf/lisa-paint
cd lisa-paint
make toolchain    # once: lisa-c's prebuilt compiler (Linux x86_64 or arm64)
make ENTRIES=/path/to/entries.tsv
```

`make` writes `build/lisapaint.dc42`: a floppy with `{T21}obj` and
`{T21}PHRASE`.

`ENTRIES` is lisa-c's table of the Lisa libraries' entry points. It's derived
from Apple's software, so it isn't in either repository. Make it from your
copy of the Lisa Workshop 3.0 library files with lisa-c's
`tools/gen_entries.py`. If a `lisa-c-private/data/entries.tsv` sits next to
this directory, it's used without being named.

If you already have a lisa-c clone next to this directory and no submodule,
that's used instead (or set `LISA_C=/path/to/lisa-c`).

To run LisaPaint you need granny-smith's headless emulator and your own Lisa
software. That means `GS_HEADLESS`, plus `LISA_ROM` (a Lisa 2 boot ROM, rev H)
and `LOS_IMAGE` (Office System 3.1 on a ProFile), or the same files in
`lisa-c/media/`. See `lisa-c/emu/env.sh`.

```bash
make run          # install it on the LOS desktop and open it (build/emu/*.png)
make serve        # run it yourself: granny-smith in your browser, LOS 3.1 booted with the
                  # LisaPaint floppy in the drive (prints the address to open)
make demo         # paint with every tool, magnify, put away, reopen, revert
                  # (build/demo/*.png)
```

lisa-c compiles every `.c` file in `src/` and `lisapaint.phrase`, and writes the
floppy (`make -C lisa-c PROG=lisapaint SRCDIR=$PWD/src TOOLNUM=21
TOOLNAME=LisaPaint tool`). On a Lisa, or in the emulator:
1. Insert the floppy and choose Repair, which registers the tool and its
   stationery.
2. Open the diskette and tear off a sheet of LisaPaint Paper.

To move to a newer lisa-c: `git -C lisa-c pull`, check that everything still
works, then commit the new `lisa-c` pointer.

## Source

| File | What |
|---|---|
| `src/main.c` | Startup, event loop, menus, the Desktop Manager (Filer) protocol |
| `src/view.c` | Window layout, palettes, scroll bars, size control, copying the picture to the screen |
| `src/tools.c` | The tools, the selection, Undo, the clipboard |
| `src/doc.c` | Reading and writing the document's diskfiles |
| `src/art.c` | Patterns, tool icons, cursors |
| `src/lisapaint.phrase` | Menus and alerts |
| `tests/demo.sh` | Scripted session in granny-smith |

## Not yet

- **The Lisa Clipboard.** Cut, Copy and Paste use LisaPaint's own clipboard
  (shared between pictures, but not with other tools).
- **Printing.**
- **A text tool.**
- **Custom icons** for the tool, pad and document (they use the generic ones).
- **Several pictures at once.** LisaPaint declines a second picture while one is open (it tells the Desktop Manager `noMoreDocs`).

## A Note on AI

This project allows the use of AI. That isn't limited to code generation or
code review: it also covers documentation and research into how the Lisa
Office System works.
There is no intention to track, at the file or commit level, which code was
written with AI assistance and which wasn't. I know that AI may be a red flag
for some people, and I fully respect that.

## License

MIT; see `LICENSE`.
