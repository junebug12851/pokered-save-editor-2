/* ============================================================================
   ff-docs.js — Fairy Fox docs-site behaviour for the Doxygen build.

   A standalone (no-module) reimplementation of the hub docs-site reader menu +
   chrome behaviour, adapted to Doxygen's DOM and this project's --ff-* tokens.
   It is a faithful hand-reimplementation of the shared reader (the standard's
   intended model), NOT a copy of the JSDoc theme's ES modules.

   Responsibilities:
     1. loadAndApply()  — apply the saved reading prefs to <html> BEFORE paint
        (runs immediately, in <head>, so there is no theme/size flash).
     2. on DOMContentLoaded:
        - measure the fixed .ff-top header stack -> --ff-header-h
        - mark the active .subnav item for the current page
        - build + wire the reader ("Aa") panel (the button is static in the header)

   Prefs live under the VERSIONED, origin-wide localStorage key
   "fairyfox:reader:b" — SHARED across every same-origin fairyfox.io site (the hub
   + each project's docs), so a reader's choice carries everywhere.
   ========================================================================== */
(function () {
  "use strict";

  var READER_KEY = "fairyfox:reader:b";
  var SIZES = [15, 16.5, 18, 20, 22]; // root px, slider 0..4 (index 1 = 16.5 default)
  var LH = { tight: 1.5, normal: 1.65, relaxed: 1.9 };
  // Reading measure. Adapted for an API-docs site: "normal" keeps the project's
  // full content width (--ff-maxw) so class/file pages are never cramped; narrow
  // caps the prose measure, wide loosens it.
  var WIDTH = { narrow: "46rem", normal: null, wide: "90rem" };
  // Curated, distinct accent hues (matches the hub reader).
  var ACCENTS = [
    ["#e0573f", "Coral"],
    ["#cf7f22", "Ochre"],
    ["#3f9e63", "Green"],
    ["#2a9ca0", "Teal"],
    ["#4478c9", "Blue"],
    ["#7d68c8", "Indigo"],
    ["#c9508a", "Rose"],
  ];
  // This project's accent-token family (the vars the reader recolours live).
  var ACCENT_VARS = [
    "--ff-accent",
    "--ff-accent-deep",
    "--ff-accent-ink",
    "--ff-link",
    "--ff-link-hover",
    "--ff-glow",
  ];
  var DEFAULTS = { theme: "system", accent: null, size: 1, lh: "normal", width: "normal" };
  var prefs = clone(DEFAULTS);

  function clone(o) {
    var r = {};
    for (var k in o) r[k] = o[k];
    return r;
  }
  function clampSize(n) {
    n = n | 0;
    return Math.max(0, Math.min(SIZES.length - 1, n));
  }
  function el(tag, attrs, html) {
    var n = document.createElement(tag);
    if (attrs) for (var k in attrs) n.setAttribute(k, attrs[k]);
    if (html != null) n.innerHTML = html;
    return n;
  }
  function here() {
    return location.pathname.split("/").pop() || "index.html";
  }
  // A code-reference (API) page — the class/struct/namespace/file/dir pages.
  // Everything else (Overview, the notes/markdown "Related Pages") is prose and
  // renders full-width with no treeview sidebar, like the sibling docs.
  function isApiPage() {
    var p = here();
    if (p === "index.html" || p === "pages.html") return false;
    if (p.indexOf("md_") === 0) return false; // generated markdown note pages
    return true;
  }

  function load() {
    try {
      var saved = JSON.parse(localStorage.getItem(READER_KEY) || "{}");
      var r = clone(DEFAULTS);
      for (var k in saved) if (k in r) r[k] = saved[k];
      return r;
    } catch (e) {
      return clone(DEFAULTS);
    }
  }
  function save() {
    try {
      localStorage.setItem(READER_KEY, JSON.stringify(prefs));
    } catch (e) {
      /* private mode / storage disabled — ignore */
    }
  }

  function applyAccent(root, hex, dark) {
    var i;
    if (!hex) {
      for (i = 0; i < ACCENT_VARS.length; i++) root.style.removeProperty(ACCENT_VARS[i]);
      return;
    }
    // Link / ink = a READABLE, still-saturated shade of the accent for the theme.
    // Mixing toward the near-white body text (the old formula) desaturated links to
    // a washed pastel on dark; mix toward pure white/black instead, and less, so the
    // accent stays vivid. Theme-aware: lighten on dark, darken on light/sepia.
    var link = dark
      ? "color-mix(in srgb, " + hex + ", #fff 28%)"
      : "color-mix(in srgb, " + hex + ", #000 16%)";
    var linkHover = dark
      ? "color-mix(in srgb, " + hex + ", #fff 48%)"
      : "color-mix(in srgb, " + hex + ", #000 30%)";
    root.style.setProperty("--ff-accent", hex);
    root.style.setProperty("--ff-accent-deep", "color-mix(in srgb, " + hex + ", #000 12%)");
    root.style.setProperty("--ff-accent-ink", link);
    root.style.setProperty("--ff-link", link);
    root.style.setProperty("--ff-link-hover", linkHover);
    root.style.setProperty("--ff-glow", "color-mix(in srgb, " + hex + " 40%, transparent)");
  }

  function apply() {
    var root = document.documentElement;
    // Drive both our data-theme (which recolours the --ff-* tokens) AND
    // doxygen-awesome's own light-mode/dark-mode class contract, so its
    // element-level dark rules (color-scheme, image inversion, ...) track the
    // reader. "system" clears both and lets the OS media query decide.
    if (prefs.theme === "system") {
      root.removeAttribute("data-theme");
      root.classList.remove("light-mode", "dark-mode");
    } else {
      root.setAttribute("data-theme", prefs.theme);
      var dark = prefs.theme === "dark";
      root.classList.toggle("dark-mode", dark);
      root.classList.toggle("light-mode", !dark); // light + sepia use the light base
    }

    var px = SIZES[clampSize(prefs.size)] + "px";
    // Scale the whole document: doxygen-awesome sizes body from --page-font-size,
    // and any rem-based bits follow the root font-size. Set both.
    root.style.fontSize = px;
    root.style.setProperty("--page-font-size", px);

    root.style.setProperty("--content-line-height", String(LH[prefs.lh] || LH.normal));

    var w = WIDTH[prefs.width];
    if (w) root.style.setProperty("--content-maxwidth", w);
    else root.style.removeProperty("--content-maxwidth"); // "normal" -> stylesheet default (--ff-maxw)

    // Effective dark? (explicit dark, or "system" on a dark OS) — so accent links
    // are lightened on dark and darkened on light/sepia.
    var effectiveDark =
      prefs.theme === "dark" ||
      (prefs.theme === "system" &&
        window.matchMedia &&
        window.matchMedia("(prefers-color-scheme: dark)").matches);
    applyAccent(root, prefs.accent, effectiveDark);
  }

  // Apply saved prefs immediately (root exists in <head>) — before first paint.
  prefs = load();
  apply();
  // Full-width (no treeview) on prose pages — set early to avoid a layout flash.
  if (!isApiPage()) {
    document.documentElement.classList.add("ff-no-sidebar");
    // Doxygen's treeview resizer persists its width in localStorage
    // ("doxygen_width"), shared across pages. On a prose page the sidebar is
    // hidden, so that resizer would record a COLLAPSED width and then shrink the
    // sidebar on the API pages too. Freeze the key on prose pages (must run
    // before doxygen's resize script) so the API-page width is preserved.
    try {
      var LS = window.localStorage;
      var realSet = LS.setItem.bind(LS);
      LS.setItem = function (k, v) {
        if (k === "doxygen_width") return;
        return realSet(k, v);
      };
    } catch (e) {
      /* storage disabled — nothing to guard */
    }
  }

  /* ---- chrome behaviour ---------------------------------------------------- */

  function measureHeader() {
    var top = document.querySelector(".ff-top");
    if (!top) return;
    var set = function () {
      document.documentElement.style.setProperty("--ff-header-h", top.offsetHeight + "px");
    };
    set();
    window.addEventListener("resize", set);
    if (document.fonts && document.fonts.ready) document.fonts.ready.then(set);
  }

  function markActiveSubnav() {
    var page = here();
    var links = document.querySelectorAll(".ff-subnav a[data-ff-page]");
    for (var i = 0; i < links.length; i++) {
      var target = links[i].getAttribute("data-ff-page");
      var on = false;
      if (target === "index.html") on = page === "index.html";
      else if (target === "pages.html") on = page === "pages.html" || page.indexOf("md_") === 0;
      else if (target === "api") on = isApiPage(); // one API item covers class/file/namespace pages
      if (on) {
        links[i].classList.add("active");
        links[i].setAttribute("aria-current", "page");
      }
    }
  }

  /* ---- reader panel -------------------------------------------------------- */

  var svg = function (inner) {
    return (
      '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" ' +
      'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' +
      inner +
      "</svg>"
    );
  };
  var ICON = {
    sun: svg(
      '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>'
    ),
    sunset: svg(
      '<path d="M17 17a5 5 0 0 0-10 0"/><path d="M12 3v3M4.5 9.5l1.5 1.5M18 11l1.5-1.5M2 17h3M19 17h3"/><path d="M3 21h18"/>'
    ),
    moon: svg('<path d="M21 12.8A9 9 0 1 1 11.2 3 7 7 0 0 0 21 12.8z"/>'),
  };
  var RESET_ICON =
    '<svg viewBox="0 0 24 24" fill="none" aria-hidden="true"><circle cx="12" cy="12" r="9" ' +
    'stroke="currentColor" stroke-width="2"/><line x1="6.6" y1="17.4" x2="17.4" y2="6.6" ' +
    'stroke="currentColor" stroke-width="2" stroke-linecap="round"/></svg>';
  var THEME_BTNS = [
    ["light", "Light", "sun"],
    ["sepia", "Sepia", "sunset"],
    ["dark", "Dark", "moon"],
  ];

  function seg(act, labelId, opts) {
    var buttons = opts
      .map(function (o) {
        return '<button type="button" data-act="' + act + '" data-val="' + o[0] + '">' + o[1] + "</button>";
      })
      .join("");
    return '<div class="ff-seg" role="group" aria-labelledby="' + labelId + '">' + buttons + "</div>";
  }

  function initReader() {
    var btn = document.querySelector(".ff-header .ff-reader-btn");
    if (!btn) return;

    var panel = el("div", {
      id: "ff-reader-panel",
      class: "ff-reader-panel",
      role: "dialog",
      "aria-label": "Reading settings",
    });

    var themeBtns = THEME_BTNS.map(function (t) {
      return (
        '<button type="button" class="ff-theme-ic" data-act="theme" data-val="' +
        t[0] +
        '" title="' +
        t[1] +
        '" aria-label="' +
        t[1] +
        '">' +
        ICON[t[2]] +
        '<span class="cap">' +
        t[1] +
        "</span></button>"
      );
    }).join("");

    var swatches =
      '<button type="button" class="ff-swatch ff-swatch-default" data-acc="" ' +
      'aria-label="Theme default accent" title="Theme default">' +
      RESET_ICON +
      "</button>" +
      ACCENTS.map(function (a) {
        return (
          '<button type="button" class="ff-swatch" data-acc="' +
          a[0] +
          '" style="--sw:' +
          a[0] +
          '" aria-label="' +
          a[1] +
          ' accent" title="' +
          a[1] +
          '"></button>'
        );
      }).join("");

    panel.innerHTML =
      '<div class="ff-rp-head"><span class="ff-rp-title">Reading settings</span>' +
      '<button type="button" class="ff-rp-close" data-act="close" aria-label="Close">&times;</button></div>' +
      '<div class="ff-rp-sec"><div class="ff-rp-schead"><span class="ff-rp-label" id="ff-rl-theme">Theme</span>' +
      '<button type="button" class="ff-auto" data-act="theme" data-val="system"><span class="dot"></span>Auto</button></div>' +
      '<div class="ff-theme-seg" role="group" aria-labelledby="ff-rl-theme">' +
      themeBtns +
      "</div></div>" +
      '<div class="ff-rp-sec"><span class="ff-rp-label" id="ff-rl-accent">Accent</span>' +
      '<div class="ff-swatches" role="group" aria-labelledby="ff-rl-accent">' +
      swatches +
      "</div></div>" +
      '<div class="ff-rp-sec"><span class="ff-rp-label" id="ff-rl-size">Text size</span>' +
      '<div class="ff-size-row"><span class="a-end a-min" aria-hidden="true">A</span>' +
      '<input type="range" class="ff-range" min="0" max="' +
      (SIZES.length - 1) +
      '" step="1" value="' +
      clampSize(prefs.size) +
      '" aria-label="Text size">' +
      '<span class="a-end a-max" aria-hidden="true">A</span></div></div>' +
      '<div class="ff-rp-sec"><span class="ff-rp-label" id="ff-rl-lh">Line spacing</span>' +
      seg("lh", "ff-rl-lh", [
        ["tight", "Tight"],
        ["normal", "Normal"],
        ["relaxed", "Relaxed"],
      ]) +
      "</div>" +
      '<div class="ff-rp-sec"><span class="ff-rp-label" id="ff-rl-width">Width</span>' +
      seg("width", "ff-rl-width", [
        ["narrow", "Narrow"],
        ["normal", "Normal"],
        ["wide", "Wide"],
      ]) +
      "</div>" +
      '<div class="ff-rp-foot"><p class="ff-rp-hint">Saved &amp; shared across Fairy&nbsp;Fox.</p>' +
      '<button type="button" class="ff-rp-reset" data-act="reset">Reset</button></div>';

    var range = panel.querySelector(".ff-range");

    function markActive() {
      var all = panel.querySelectorAll("[data-act], .ff-swatch");
      for (var i = 0; i < all.length; i++) {
        var b = all[i];
        var act = b.getAttribute("data-act");
        var on = null;
        if (b.classList.contains("ff-swatch")) on = b.getAttribute("data-acc") === (prefs.accent || "");
        else if (act === "theme" || act === "lh" || act === "width") on = b.getAttribute("data-val") === prefs[act];
        if (on !== null) b.setAttribute("aria-pressed", on ? "true" : "false");
      }
      if (range) range.value = clampSize(prefs.size);
    }
    markActive();

    function setOpen(open) {
      panel.classList.toggle("open", open);
      btn.setAttribute("aria-expanded", open ? "true" : "false");
      if (open) {
        var c = panel.querySelector(".ff-rp-close");
        if (c) c.focus();
      }
    }

    panel.addEventListener("click", function (e) {
      var b = e.target.closest("button");
      if (!b || !panel.contains(b)) return;
      if (b.classList.contains("ff-swatch")) {
        prefs.accent = b.getAttribute("data-acc") || null;
      } else {
        var act = b.getAttribute("data-act");
        if (act === "close") {
          setOpen(false);
          btn.focus();
          return;
        }
        if (act === "reset") prefs = clone(DEFAULTS);
        else if (act === "theme" || act === "lh" || act === "width") prefs[act] = b.getAttribute("data-val");
        else return;
      }
      apply();
      save();
      markActive();
    });
    range.addEventListener("input", function () {
      prefs.size = clampSize(+range.value);
      apply();
      save();
    });

    btn.addEventListener("click", function (e) {
      e.stopPropagation();
      setOpen(!panel.classList.contains("open"));
    });
    document.addEventListener("click", function (e) {
      if (panel.classList.contains("open") && !panel.contains(e.target) && e.target !== btn && !btn.contains(e.target))
        setOpen(false);
    });
    document.addEventListener("keydown", function (e) {
      if (e.key === "Escape" && panel.classList.contains("open")) {
        setOpen(false);
        btn.focus();
      }
    });

    document.body.appendChild(panel);
  }

  function run() {
    var root = document.documentElement;
    if (root.hasAttribute("data-ff-themed")) return;
    root.setAttribute("data-ff-themed", "");
    prefs = load();
    apply();
    measureHeader();
    markActiveSubnav();
    initReader();
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", run);
  else run();
})();
