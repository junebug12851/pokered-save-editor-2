#!/usr/bin/env python3
"""Build the themed /screenshots/ gallery page for the Pages site.

It wears the vendored fairyfox chrome bundle (../main.css, ../reader.js, ../nav.js,
../coins.js, ../doxygen-awesome.css, ../fairyfox-doxygen.css, self-hosted ../fonts.css), so the
gallery is seamless with the rest of the Fairy Fox docs site rather than a bare page.

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


# ── Vendored fairyfox chrome bundle (chrome VERSION 2.0.0), hard-coded to match
#    docs/fairyfox/header.html + footer.html. Plain strings (literal JS braces) so the
#    f-string template below interpolates them without brace-escaping. Assets resolve one
#    level up (../) because the gallery lives at /screenshots/. Self-hosted fonts (deviation).
FF_HEAD = """  <meta name="theme-color" content="#ef6149" media="(prefers-color-scheme: light)"/>
  <meta name="theme-color" content="#191116" media="(prefers-color-scheme: dark)"/>
  <script>
(function(){try{var p=JSON.parse(localStorage.getItem("fairyfox:reader:b")||"{}"),r=document.documentElement,
S=[15,16.5,18,20,22];
if(p.theme&&p.theme!=="system")r.setAttribute("data-theme",p.theme);
if(p.size!=null)r.style.fontSize=S[Math.max(0,Math.min(S.length-1,p.size|0))]+"px";
if(p.accent){var h=p.accent,ink="color-mix(in srgb, "+h+", var(--text) 42%)";
r.style.setProperty("--accent",h);r.style.setProperty("--violet",h);
r.style.setProperty("--violet-deep","color-mix(in srgb, "+h+", #000 12%)");
r.style.setProperty("--accent-ink",ink);r.style.setProperty("--link",ink);
r.style.setProperty("--link-hover","color-mix(in srgb, "+h+", var(--text) 26%)");
r.style.setProperty("--glow","color-mix(in srgb, "+h+" 40%, transparent)");}}catch(e){}})();
</script>
  <link href="../fonts.css" rel="stylesheet" type="text/css"/>
  <link href="../doxygen-awesome.css" rel="stylesheet" type="text/css"/>
  <link href="../main.css" rel="stylesheet" type="text/css"/>
  <link href="../fairyfox-doxygen.css" rel="stylesheet" type="text/css"/>"""

FF_HEADER = """<a class="ff-skip" href="#gallery">Skip to content</a>
<header class="site-header">
  <div class="wrap">
    <a class="brand" href="https://fairyfox.io/">
      <img class="brand-logo" src="https://fairyfox.io/assets/icons/fox.png" alt="" aria-hidden="true"/>
      <span class="brand-name">Fairy&nbsp;Fox</span>
    </a>
    <button class="nav-toggle" aria-label="Toggle navigation" aria-expanded="false"><span></span><span></span><span></span></button>
    <nav class="nav" aria-label="Primary">
      <a href="https://fairyfox.io/">Home</a>
      <a href="https://fairyfox.io/projects/" class="active" aria-current="page">Projects</a>
      <details class="dd"><summary>Farms</summary><div class="dd-panel"><div class="dd-group"><div class="dd-links">
        <a href="https://fairyfox.io/stories/">Stories</a>
        <a href="https://fairyfox.io/games/">Games</a>
      </div></div></div></details>
      <a href="https://fairyfox.io/docs/">Docs</a>
      <a href="https://fairyfox.io/blog/">Updates</a>
      <a href="https://fairyfox.io/about/">About</a>
    </nav>
  </div>
</header>
<nav class="subnav" aria-label="Section">
  <div class="wrap">
    <a href="../index.html">Overview</a>
    <a href="../pse_notes.html">Notes</a>
    <a href="../annotated.html">API Reference</a>
    <a href="index.html" class="active" aria-current="page">Screenshots</a>
    <a class="ext" href="https://github.com/1fairyfox/pokered-save-editor-2">Repository&nbsp;&#8599;</a>
  </div>
</nav>"""

FF_FOOTER = """<footer class="site-footer">
  <div class="wrap">
    <div class="footer-brand">
      <a class="brand" href="https://fairyfox.io/">
        <img class="brand-logo" src="https://fairyfox.io/assets/icons/fox.png" alt="" aria-hidden="true"/>
        <span class="brand-name">Fairy Fox</span>
      </a>
      <p>The project hub and documentation library for Fairy Fox's software work.</p>
    </div>
    <div class="footer-col">
      <h4>Explore</h4>
      <a href="https://fairyfox.io/projects/">Projects</a>
      <a href="https://fairyfox.io/docs/">Documentation</a>
      <a href="https://fairyfox.io/blog/">Updates</a>
      <a href="https://fairyfox.io/about/">About</a>
    </div>
    <div class="footer-col">
      <h4>This project</h4>
      <a href="https://fairyfox.io/pokered-save-editor-2/">Pokered Save Editor 2</a>
      <a href="https://github.com/1fairyfox/pokered-save-editor-2">Repository&nbsp;&#8599;</a>
    </div>
    <div class="footer-col">
      <h4>Elsewhere</h4>
      <a href="https://github.com/1fairyfox">GitHub&nbsp;&#8599;</a>
      <a href="https://fairyfox.io/feed.xml">Atom feed</a>
    </div>
  </div>
  <div class="footer-bar">
    <div class="wrap">
      <span>&#169; Fairy Fox</span>
      <span>Built with the fairyfox docs-site standard</span>
      <span class="spacer"></span>
      <a href="https://github.com/1fairyfox">@1fairyfox</a>
    </div>
  </div>
</footer>"""

FF_SCRIPTS = """<script src="../nav.js" defer></script>
<script src="../reader.js" defer></script>
<script src="../coins.js" defer></script>
<script>
(function(){var r=document.documentElement;
function sync(){var t=r.getAttribute("data-theme")||"system";
var dark=t==="dark"||(t==="system"&&window.matchMedia&&window.matchMedia("(prefers-color-scheme: dark)").matches);
r.classList.toggle("dark-mode",dark);r.classList.toggle("light-mode",!dark);}
sync();try{new MutationObserver(sync).observe(r,{attributes:true,attributeFilter:["data-theme"]});
if(window.matchMedia){var mq=window.matchMedia("(prefers-color-scheme: dark)");
(mq.addEventListener?mq.addEventListener.bind(mq,"change"):mq.addListener.bind(mq))(sync);}}catch(e){}})();
</script>"""


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

    doc = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <meta http-equiv="X-UA-Compatible" content="IE=11"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Screenshots: Pokered Save Editor 2</title>
  <link rel="icon" href="../pse-logo.png"/>
{FF_HEAD}
</head>
<body>
{FF_HEADER}
<main id="gallery" class="ff-gallery" tabindex="-1">
  <h1 class="ff-gallery-title">Screenshots</h1>
  <p class="ff-gallery-lead">The Pokered Save Editor&nbsp;2 UI, auto-captured from <code>main</code>. Click any shot to open it full-size. &middot; <a href="../index.html">&#8592; Back to docs</a></p>
{chr(10).join(sections)}
</main>
{FF_FOOTER}
{FF_SCRIPTS}
</body>
</html>
"""
    (out / "index.html").write_text(doc, encoding="utf-8")
    print(f"gallery: {total} screenshots in {len(sections)} section(s) -> {out/'index.html'}")


if __name__ == "__main__":
    main()
