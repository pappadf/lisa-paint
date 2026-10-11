#!/usr/bin/env python3
"""Write the GitHub Pages index for a LisaPaint release (.github/workflows/release.yml).

    pages.py SITE_URL TAG > index.html

The page links to granny-smith's web build with a Lisa 2, the Office System
3.1 ProFile and LisaPaint's floppy (SITE_URL/lisapaint.dc42).  The ROM and
the ProFile are copies on archive.org, as on granny-smith's Boot Shelf.  The
emulator is a fixed granny-smith release, GS_VERSION (default v0.9.3, the one
lisa-c's emu/serve.sh uses), never the moving staging build.
"""
import html
import os
import sys
import urllib.parse

GS_VERSION = os.environ.get('GS_VERSION') or 'v0.9.3'
EMULATOR = f'https://pappadf.github.io/gs-pages/{GS_VERSION}/'
ROMS = ['https://ia600103.us.archive.org/view_archive.php?archive=/21/items/lisa-software/'
        'Lisa%20Software.zip&file=Lisa%20Software%2Ffirmware%2F341-0175-H.BIN',
        'https://ia600103.us.archive.org/view_archive.php?archive=/21/items/lisa-software/'
        'Lisa%20Software.zip&file=Lisa%20Software%2Ffirmware%2F341-0176-H.BIN']
LOS = ('https://ia600806.us.archive.org/view_archive.php?archive=/22/items/'
       'apple-lisa-profile-hd-disk-images-for-lisaem-and-idle-lisa-office-system-3.1-lis/'
       'IDLE_LOS3.1-after1stBoot.zip&file=profile.raw')


def boot_url(floppy):
    q = [('ROM', r) for r in ROMS] + [('HD0', LOS), ('FD0', floppy)]
    return EMULATOR + '?' + '&'.join(k + '=' + urllib.parse.quote(v, safe='')
                                     for k, v in q)


def main():
    site, tag = sys.argv[1].rstrip('/'), sys.argv[2]
    floppy = site + '/lisapaint.dc42'
    boot = html.escape(boot_url(floppy))
    print(f'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>LisaPaint</title>
<style>
  :root {{ --bg: #f4f1ea; --fg: #1d1d1b; --muted: #5d5b55; --line: #cfc9bb; --accent: #1d1d1b; }}
  @media (prefers-color-scheme: dark) {{
    :root {{ --bg: #171715; --fg: #ecebe6; --muted: #a8a59c; --line: #3a3934; --accent: #ecebe6; }}
  }}
  body {{ background: var(--bg); color: var(--fg); margin: 0;
         font: 16px/1.55 ui-sans-serif, system-ui, -apple-system, "Segoe UI", sans-serif; }}
  main {{ max-width: 46rem; margin: 0 auto; padding: 3rem 16px; }}
  h1 {{ font-size: 2rem; margin: 0 0 .25rem; letter-spacing: -.01em; }}
  .ver {{ color: var(--muted); margin: 0 0 1.5rem; }}
  img {{ width: 100%; height: auto; image-rendering: pixelated; border: 1px solid var(--line); }}
  .boot {{ display: inline-block; margin: 1.5rem 0 .5rem; padding: .7rem 1.4rem; border: 2px solid var(--accent);
          color: var(--accent); text-decoration: none; font-weight: 600; }}
  .boot:hover {{ background: var(--accent); color: var(--bg); }}
  ol {{ padding-left: 1.2rem; }}
  .small {{ color: var(--muted); font-size: .9rem; }}
  a {{ color: inherit; }}
</style>
</head>
<body>
<main>
  <h1>LisaPaint</h1>
  <p class="ver">{html.escape(tag)} &middot; a pixel painting tool for the Apple Lisa Office System</p>
  <img src="https://raw.githubusercontent.com/pappadf/lisa-paint/{html.escape(tag)}/docs/screenshot2.png"
       alt="LisaPaint showing the Astronaut picture on the Lisa desktop" width="1440" height="1092">
  <a class="boot" href="{boot}" target="_blank" rel="noopener">Boot it in your browser</a>
  <p class="small">Opens the <a href="https://github.com/pappadf/granny-smith">granny-smith</a>
  emulator ({html.escape(GS_VERSION)}): a Lisa 2 starting the Office System 3.1, with the
  LisaPaint floppy in the drive (the ROM and the Office System disk come from archive.org; the first
  start takes a little while).</p>
  <ol>
    <li>Open the <b>LisaPaint</b> diskette.</li>
    <li>Open the <b>Astronaut</b> picture, or double-click the <b>LisaPaint Paper</b> pad to tear off a
    new sheet and open that.</li>
  </ol>
  <p class="small">The floppy: <a href="lisapaint.dc42">lisapaint.dc42</a> (DiskCopy 4.2, 400K).
  Source and earlier versions: <a href="https://github.com/pappadf/lisa-paint">github.com/pappadf/lisa-paint</a>.</p>
</main>
</body>
</html>''')


if __name__ == '__main__':
    main()
