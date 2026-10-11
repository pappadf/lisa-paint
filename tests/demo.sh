#!/usr/bin/env bash
# Install LisaPaint from its floppy on the LOS 3.1 desktop, open a new
# picture, paint with several tools, magnify, set the picture aside and open
# it again (the picture must come back as it was).
#
#   tests/demo.sh floppy.dc42 [out-dir]
#
# Uses lisa-c's emulator settings (LISA_C/emu/env.sh: gs-headless, the Lisa
# ROM and the Office System image).  Screen positions are those of a
# single-tool diskette and LisaPaint's window (deterministic).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -z "${LISA_C:-}" ]; then          # the submodule, else a lisa-c next to this project
    if [ -f "$HERE/../lisa-c/Makefile" ]; then LISA_C="$HERE/../lisa-c"; else LISA_C="$HERE/../../lisa-c"; fi
fi
source "$LISA_C/emu/env.sh"
need_emulator
need "$LISA_ROM" "Lisa ROM (LISA_ROM)"
need "$LOS_IMAGE" "Office System ProFile image (LOS_IMAGE)"
OUT="$(realpath -m "${2:-$HERE/../build/demo}")"
mkdir -p "$OUT"
cp "$LOS_IMAGE" "$OUT/profile.image"
cp "$1" "$OUT/floppy.dc42"

# Window content origin on the screen, and helpers to address the palettes.
OX=2; OY=33
tool_xy() { local t=$1; echo "$((OX + 2 + (t % 2) * 30 + 15)) $((OY + 2 + (t / 2) * 20 + 10))"; }
pat_xy()  { local k=$1; echo "$((132 + (k % 19) * 25)) $((271 + (k / 19) * 17))"; }

S="$OUT/demo.script"
emit() { echo "$*" >> "$S"; }
move() { emit "machine.adb.mouse.move $1 $2 \"global\""; emit "scheduler.run ${3:-400000}"; }
press() { emit "machine.adb.mouse.click true"; emit "scheduler.run 200000"; }
release() { emit "machine.adb.mouse.click false"; emit "scheduler.run 2000000"; }
click() { move "$1" "$2" 2000000; press; release; }
dclick() {  # the second press must follow the first quickly
    move "$1" "$2" 2000000
    emit "machine.adb.mouse.click true"; emit "scheduler.run 80000"
    emit "machine.adb.mouse.click false"; emit "scheduler.run 120000"
    emit "machine.adb.mouse.click true"; emit "scheduler.run 80000"
    emit "machine.adb.mouse.click false"
}
# drag x1 y1 x2 y2 [steps]: press at the first point, move in steps, release
drag() {
    local x1=$1 y1=$2 x2=$3 y2=$4 n=${5:-8}
    move "$x1" "$y1" 2000000; press
    for i in $(seq 1 "$n"); do move $((x1 + (x2 - x1) * i / n)) $((y1 + (y2 - y1) * i / n)) 600000; done
    release
}
path() {    # path x1 y1 x2 y2 ... : press, visit every point, release
    move "$1" "$2" 2000000; press; shift 2
    while [ $# -ge 2 ]; do move "$1" "$2" 600000; shift 2; done
    release
}
shot() { emit "machine.screen.save \"$OUT/$1.png\""; }
menu() {    # menu title-x item-number
    move "$1" 7 2000000; press
    move "$1" $((7 + 16 * $2)) 2000000; move $(( $1 + 4 )) $((7 + 16 * $2)) 2000000
    shot "menu-$1-$2"
    release
}

: > "$S"
emit "machine.boot model=\"lisa\" ram=2048 rom=\"$LISA_ROM\""
emit "machine.hd.attach \"$OUT/profile.image\""
emit 'assert machine.hd.pram_init(2, false) "pram_init failed"'
emit "scheduler.run 60000000"
emit "machine.floppy.drive.0.insert \"$OUT/floppy.dc42\""
emit "scheduler.run 40000000"
dclick 181 336; emit "scheduler.run 40000000"          # open the diskette
dclick 70 58; emit "scheduler.run 60000000"            # tear off LisaPaint Paper
shot 0-torn-off
if [ -n "${EXPLORE:-}" ]; then emit quit; "$GS_HEADLESS" --speed=turbo rom="$LISA_ROM" model=lisa script="$S" > "$OUT/demo.log" 2>&1; exit; fi
dclick 36 55; emit "scheduler.run 120000000"           # open the new document
shot 1-opened

click $(pat_xy 21)                                     # checkerboard
click $(tool_xy 12)                                    # filled oval
drag 110 60 250 150
click $(pat_xy 13)                                     # thick diagonal
click $(tool_xy 8)                                     # filled rectangle
drag 290 70 420 160
click $(tool_xy 4)                                     # pencil
path 450 60 470 120 490 60 510 120 530 60 550 120
click $(tool_xy 6)                                     # line
drag 100 200 590 175
click $(tool_xy 9)                                     # rounded rectangle
drag 440 140 600 220
click $(pat_xy 2)                                      # gray
click $(tool_xy 1)                                     # paint bucket
click 520 185
click $(pat_xy 0)                                      # black
click $(tool_xy 3)                                     # brush
path 120 175 160 190 200 172 240 192 280 176
click $(tool_xy 2)                                     # spray
drag 330 190 400 200 6
shot 2-painted

click $(tool_xy 0)                                     # selection
drag 140 80 220 130
emit "scheduler.run 6000000"
shot 3-selected
drag 180 105 230 200                                   # move it
shot 4-moved
menu 222 4                                             # Selection > Flip Horizontal
shot 5-flipped
menu 165 1                                             # Edit > Undo Last Change
shot 6-undone
menu 440 2                                             # Page Layout > Magnify Pixels
shot 7-magnified
menu 440 1                                             # Page Layout > Show Actual Size

menu 95 4                                              # File/Print > Save & Put Away
emit "scheduler.run 80000000"
shot 8-put-away
dclick 36 55                                           # open it again from the diskette
emit "scheduler.run 120000000"
shot 9-reopened
click $(tool_xy 6)                                     # line: a change to revert
drag 120 230 600 100
shot 10-changed
menu 95 6                                              # File/Print > Revert to Previous Version
emit "scheduler.run 20000000"
shot 11-revert-alert
click 607 134                                          # OK
emit "scheduler.run 20000000"
shot 12-reverted
move 707 290 2000000; press; emit "scheduler.run 6000000"; release   # hold the down arrow
shot 13-scrolled-down
click 400 306                                          # gray part of the horizontal bar
shot 14-paged-right
drag 708 306 560 250 6                                 # the size control
emit "scheduler.run 20000000"
shot 15-resized
emit "quit"

"$GS_HEADLESS" --speed=turbo rom="$LISA_ROM" model=lisa script="$S" > "$OUT/demo.log" 2>&1
grep -E 'rror' "$OUT/demo.log" || true
ls "$OUT"/*.png | wc -l
