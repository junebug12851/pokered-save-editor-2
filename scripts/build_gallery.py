#!/usr/bin/env python3
"""Build the themed /screenshots/ gallery page for the Pages site.

It reuses the docs' own chrome + stylesheet (../fairyfox-doxygen.css,
../doxygen-awesome.css, ../fonts.css, ../ff-docs.js), so the gallery is seamless
with the rest of the Fairy Fox docs site rather than a bare unstyled page.

Usage:
    build_gallery.py <screenshots-src-root> <output-dir>

<screenshots-src-root> holds the capture groups (screens/, editor/); their PNGs
are copied flat into <output-dir> and laid out in a responsive card grid, grouped
by section. <output-dir> is the site's /screenshots/ directory (one level below
the docs root, so the ../ asset links resolve).
"""
import html
import shutil
import sys
from pathlib import Path

# Capture groups -> section heading (in display order).
GROUPS = [("screens", "Screens"), ("editor", "Editor")]

# Small caption fix-ups after generic humanising (title-cased words).
FIXUPS = {
    "Dvev": "DV / EV", "Npc": "NPC", "Hm": "HM", "Ui": "UI", "Db": "DB",
    "Pokedex": "Pokédex", "Pokemon": "Pokémon", "Id": "ID", "Pp": "PP",
}


def caption(stem: str) -> str:
    words = stem.replace("-", " ").replace("_", " ").split()
    out = []
    for w in words:
        t = w[:1].upper() + w[1:]
        out.append(FIXUPS.get(t, t))
    return " ".join(out)


def cards(files):
    rows = []
    for f in files:
        name = html.escape(f.name)
        cap = html.escape(caption(f.stem))
        rows.append(
            f'      <figure class="ff-card"><a href="{name}" target="_blank" rel="noopener">'
            f'<img src="{name}" loading="lazy" alt="{cap}"/></a>'
            f'<figcaption>{cap}</figcaption></figure>'
        )
    return "\n".join(rows)


def nav_link(label, href, active=False, ext=False):
    cls = []
    if active:
        cls.append("active")
    if ext:
        cls.append("ext")
    attr = f' class="{" ".join(cls)}"' if cls else ""
    cur = ' aria-current="page"' if active else ""
    return f'<a{attr}{cur} href="{href}">{label}</a>'


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)

    sections = []
    total = 0
    for folder, heading in GROUPS:
        d = src / folder
        if not d.is_dir():
            continue
        files = sorted(p for p in d.glob("*.png"))
        if not files:
            continue
        for f in files:
            shutil.copy2(f, out / f.name)
        total += len(files)
        sections.append(
            f'    <section>\n      <h2>{html.escape(heading)}</h2>\n'
            f'      <div class="ff-grid">\n{cards(files)}\n      </div>\n    </section>'
        )

    primary = "".join(
        "\n        " + nav_link(l, h, active=(l == "Docs"))
        for l, h in [
            ("Home", "https://fairyfox.io/"), ("Projects", "https://fairyfox.io/projects/"),
            ("Games", "https://fairyfox.io/games/"), ("Docs", "https://fairyfox.io/docs/"),
            ("Updates", "https://fairyfox.io/blog/"), ("About", "https://fairyfox.io/about/"),
        ]
    )
    repo = "https://github.com/junebug12851/pokered-save-editor-2"
    sub = "".join(
        "\n      " + nav_link(*a)
        for a in [
            ("Overview", "../index.html"), ("Notes", "../pages.html"), ("API", "../annotated.html"),
        ]
    )

    doc = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <meta http-equiv="X-UA-Compatible" content="IE=11"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Screenshots: Pokered Save Editor 2</title>
  <meta name="theme-color" content="#ef6149" media="(prefers-color-scheme: light)"/>
  <meta name="theme-color" content="#191116" media="(prefers-color-scheme: dark)"/>
  <link rel="icon" href="../pse-logo.png"/>
  <link href="../fonts.css" rel="stylesheet" type="text/css"/>
  <script type="text/javascript" src="../ff-docs.js"></script>
  <link href="../doxygen-awesome.css" rel="stylesheet" type="text/css"/>
  <link href="../fairyfox-doxygen.css" rel="stylesheet" type="text/css"/>
</head>
<body>
<a class="ff-skip" href="#gallery">Skip to content</a>
<div class="ff-top">
  <header class="site-header ff-header" role="banner">
    <div class="wrap">
      <a class="brand" href="https://fairyfox.io/">
        <img class="brand-logo" src="https://fairyfox.io/assets/icons/fox.png" alt="" aria-hidden="true"/>
        <span class="brand-name">Fairy&nbsp;Fox</span>
      </a>
      <nav class="nav" aria-label="Fairy Fox">{primary}
      </nav>
      <button class="ff-reader-btn" type="button" aria-label="Reading settings"
        aria-haspopup="dialog" aria-expanded="false" aria-controls="ff-reader-panel" title="Reading settings">
        <span class="aa-lg">A</span><span class="aa-sm">a</span>
      </button>
    </div>
  </header>
  <nav class="subnav ff-subnav" aria-label="Pokered Save Editor 2 sections">
    <div class="wrap">
      <a class="sub-brand" href="../index.html">Pokered Save Editor&nbsp;2</a>{sub}
      <a class="active" aria-current="page" href="index.html">Screenshots</a>
      <span class="sep" aria-hidden="true"></span>
      <a class="ext" href="{repo}">Repository&nbsp;&#8599;</a>
    </div>
  </nav>
</div>

<main id="gallery" class="ff-gallery" tabindex="-1">
  <h1 class="ff-gallery-title">Screenshots</h1>
  <p class="ff-gallery-lead">The Pokered Save Editor&nbsp;2 UI, auto-captured from <code>main</code>. Click any shot to open it full-size. &middot; <a href="../index.html">&#8592; Back to docs</a></p>
{chr(10).join(sections)}
</main>

<footer class="ff-foot">
  <div class="ff-foot-wrap">
    <div class="ff-foot-brand">
      <a class="brand" href="https://fairyfox.io/">
        <img class="brand-logo" src="https://fairyfox.io/assets/icons/fox.png" alt="" aria-hidden="true"/>
        <span class="brand-name">Fairy&nbsp;Fox</span>
      </a>
      <p>Documentation for Pokered Save Editor&nbsp;2 &mdash; a project under Fairy Fox.</p>
    </div>
    <nav class="ff-foot-col" aria-label="Fairy Fox">
      <h2>Explore</h2>
      <a href="https://fairyfox.io/projects/">Projects</a>
      <a href="https://fairyfox.io/docs/">Documentation</a>
      <a href="https://fairyfox.io/blog/">Updates</a>
      <a href="https://fairyfox.io/about/">About</a>
    </nav>
    <nav class="ff-foot-col" aria-label="This project">
      <h2>This project</h2>
      <a href="../index.html">Docs home</a>
      <a href="../pages.html">Project notes</a>
      <a href="index.html">Screenshots</a>
      <a href="{repo}">Repository&nbsp;&#8599;</a>
    </nav>
  </div>
  <div class="ff-foot-bar">
    <div class="ff-foot-wrap">
      <span>&#169; Fairy Fox</span>
      <span>A project under <a href="https://fairyfox.io/">Fairy&nbsp;Fox</a></span>
      <span class="spacer"></span>
      <a href="https://github.com/junebug12851">@junebug12851</a>
    </div>
  </div>
</footer>
</body>
</html>
"""
    (out / "index.html").write_text(doc, encoding="utf-8")
    print(f"gallery: {total} screenshots in {len(sections)} section(s) -> {out/'index.html'}")


if __name__ == "__main__":
    main()
