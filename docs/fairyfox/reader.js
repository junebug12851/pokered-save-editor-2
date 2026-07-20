// reader.js — the reading-appearance menu (the "Aa" button + panel): theme, accent
// colour, text size, line spacing and reading width, tuned live and remembered.
//
// Theme is a row of weather/time icon buttons (sun = Light, sunset = Sepia, moon =
// Dark) plus an "Auto" toggle in the section header (follow the OS). Accent is a row
// of colour dots with a "reset to theme default" swatch. Text size is a slider (small
// A → large A) that scales the document ROOT font-size, so it resizes the whole UI on
// every page. Line spacing drives body line-height; width caps the reading measure.
//
// Text size, theme and accent apply everywhere. Line spacing + width are READING-PAGE
// ONLY: they take effect (and their controls un-lock) only on a page meant to be READ —
// `<html data-read>` (a note, a legal page, a guide) or `<html data-story>` (a book/
// chapter) — so index/list/API/category/sidebar pages keep the normal, designed measure.
// Off a reading page the two controls sit visible-but-disabled with a note; the saved
// values are kept, just not applied. The signal is an attribute on <html> so the
// pre-paint <head> script (which runs before <body>) can read it too.
//
// Prefs live under a VERSIONED origin-wide key ("fairyfox:reader:b"), shared across
// every same-origin fairyfox.io site. Early apply is inline in <head> (no flash).
(function () {
  "use strict";

  var KEY = "fairyfox:reader:b";
  var SIZES = [15, 16.5, 18, 20, 22];          // root px, slider 0..4
  var LH = { tight: 1.5, normal: 1.65, relaxed: 1.9 };
  var WIDTH = { narrow: "38rem", normal: "46rem", wide: "58rem" };
  // Curated, distinct accent hues (refined — not neon, no duplicate oranges).
  var ACCENTS = [
    ["#e0573f", "Coral"], ["#cf7f22", "Ochre"], ["#3f9e63", "Green"],
    ["#2a9ca0", "Teal"], ["#4478c9", "Blue"], ["#7d68c8", "Indigo"], ["#c9508a", "Rose"],
  ];
  var ACCENT_VARS = ["--accent", "--violet", "--violet-deep", "--accent-ink", "--link", "--link-hover", "--glow"];
  var DEFAULTS = { theme: "system", accent: null, size: 1, lh: "normal", width: "normal" };

  var svg = function (inner) {
    return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' + inner + "</svg>";
  };
  var ICON = {
    sun: svg('<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>'),
    sunset: svg('<path d="M17 17a5 5 0 0 0-10 0"/><path d="M12 3v3M4.5 9.5l1.5 1.5M18 11l1.5-1.5M2 17h3M19 17h3"/><path d="M3 21h18"/>'),
    moon: svg('<path d="M21 12.8A9 9 0 1 1 11.2 3 7 7 0 0 0 21 12.8z"/>'),
  };
  var RESET_ICON = '<svg viewBox="0 0 24 24" fill="none" aria-hidden="true"><circle cx="12" cy="12" r="9" stroke="currentColor" stroke-width="2"/><line x1="6.6" y1="17.4" x2="17.4" y2="6.6" stroke="currentColor" stroke-width="2" stroke-linecap="round"/></svg>';
  var THEME_BTNS = [["light", "Light", "sun"], ["sepia", "Sepia", "sunset"], ["dark", "Dark", "moon"]];

  var prefs = Object.assign({}, DEFAULTS);
  function clampSize(n) { return Math.max(0, Math.min(SIZES.length - 1, n | 0)); }
  // Readable pages opt in via `data-read` (any page meant to be READ — a note, a legal
  // page, a guide) or `data-story` (a book/chapter). Only there do line spacing + width
  // apply and their controls un-lock; on index/list/API/category/sidebar pages reading
  // uses the normal defaults. `data-story` implies readable (kept for back-compat).
  function isReadable() {
    var r = document.documentElement;
    return r.hasAttribute("data-read") || r.hasAttribute("data-story");
  }

  function load() {
    try { return Object.assign({}, DEFAULTS, JSON.parse(localStorage.getItem(KEY) || "{}")); }
    catch (e) { return Object.assign({}, DEFAULTS); }
  }
  function save() { try { localStorage.setItem(KEY, JSON.stringify(prefs)); } catch (e) { /* ignore */ } }

  function applyAccent(root, hex) {
    if (!hex) { ACCENT_VARS.forEach(function (v) { root.style.removeProperty(v); }); return; }
    var ink = "color-mix(in srgb, " + hex + ", var(--text) 42%)";
    root.style.setProperty("--accent", hex);
    root.style.setProperty("--violet", hex);
    root.style.setProperty("--violet-deep", "color-mix(in srgb, " + hex + ", #000 12%)");
    root.style.setProperty("--accent-ink", ink);
    root.style.setProperty("--link", ink);
    root.style.setProperty("--link-hover", "color-mix(in srgb, " + hex + ", var(--text) 26%)");
    root.style.setProperty("--glow", "color-mix(in srgb, " + hex + " 40%, transparent)");
  }
  function apply() {
    var root = document.documentElement;
    if (prefs.theme === "system") root.removeAttribute("data-theme");
    else root.setAttribute("data-theme", prefs.theme);
    root.style.fontSize = SIZES[clampSize(prefs.size)] + "px";
    // Line spacing + width are story-only. Off a story, drop the overrides so the
    // designed defaults (from the stylesheet) stand.
    if (isReadable()) {
      root.style.setProperty("--reading-lh", String(LH[prefs.lh] || LH.normal));
      root.style.setProperty("--reading-width", WIDTH[prefs.width] || WIDTH.normal);
    } else {
      root.style.removeProperty("--reading-lh");
      root.style.removeProperty("--reading-width");
    }
    applyAccent(root, prefs.accent);
  }

  function el(tag, attrs, html) {
    var n = document.createElement(tag);
    if (attrs) Object.keys(attrs).forEach(function (k) { n.setAttribute(k, attrs[k]); });
    if (html != null) n.innerHTML = html;
    return n;
  }
  function seg(act, labelId, opts) {
    return '<div class="ff-seg" role="group" aria-labelledby="' + labelId + '">' +
      opts.map(function (o) {
        return '<button type="button" data-act="' + act + '" data-val="' + o[0] + '">' + o[1] + "</button>";
      }).join("") + "</div>";
  }

  function init() {
    prefs = load();
    apply();

    var btn = el("button", {
      class: "ff-reader-btn", type: "button", "aria-label": "Reading settings",
      "aria-haspopup": "dialog", "aria-expanded": "false", "aria-controls": "ff-reader-panel", title: "Reading settings",
    });
    btn.innerHTML = '<span class="aa-lg">A</span><span class="aa-sm">a</span>';

    var panel = el("div", { id: "ff-reader-panel", class: "ff-reader-panel", role: "dialog", "aria-label": "Reading settings" });

    var themeBtns = THEME_BTNS.map(function (t) {
      return '<button type="button" class="ff-theme-ic" data-act="theme" data-val="' + t[0] + '" title="' + t[1] + '" aria-label="' + t[1] + '">' +
        ICON[t[2]] + '<span class="cap">' + t[1] + "</span></button>";
    }).join("");

    var swatches = '<button type="button" class="ff-swatch ff-swatch-default" data-acc="" aria-label="Theme default accent" title="Theme default">' + RESET_ICON + "</button>" +
      ACCENTS.map(function (a) {
        return '<button type="button" class="ff-swatch" data-acc="' + a[0] + '" style="--sw:' + a[0] + '" aria-label="' + a[1] + ' accent" title="' + a[1] + '"></button>';
      }).join("");

    panel.innerHTML =
      '<div class="ff-rp-head"><span class="ff-rp-title">Reading settings</span>' +
      '<button type="button" class="ff-rp-close" data-act="close" aria-label="Close">×</button></div>' +
      '<div class="ff-rp-sec"><div class="ff-rp-schead"><span class="ff-rp-label" id="ff-rl-theme">Theme</span>' +
      '<button type="button" class="ff-auto" data-act="theme" data-val="system"><span class="dot"></span>Auto</button></div>' +
      '<div class="ff-theme-seg" role="group" aria-labelledby="ff-rl-theme">' + themeBtns + "</div></div>" +
      '<div class="ff-rp-sec"><span class="ff-rp-label" id="ff-rl-accent">Accent</span>' +
      '<div class="ff-swatches" role="group" aria-labelledby="ff-rl-accent">' + swatches + "</div></div>" +
      '<div class="ff-rp-sec"><span class="ff-rp-label" id="ff-rl-size">Text size</span>' +
      '<div class="ff-size-row"><span class="a-end a-min" aria-hidden="true">A</span>' +
      '<input type="range" class="ff-range" min="0" max="' + (SIZES.length - 1) + '" step="1" value="' + clampSize(prefs.size) + '" aria-label="Text size">' +
      '<span class="a-end a-max" aria-hidden="true">A</span></div></div>' +
      '<div class="ff-rp-sec ff-rp-lockable"><span class="ff-rp-label" id="ff-rl-lh">Line spacing</span>' +
      seg("lh", "ff-rl-lh", [["tight", "Tight"], ["normal", "Normal"], ["relaxed", "Relaxed"]]) +
      '<p class="ff-rp-note">Enables on reading pages.</p></div>' +
      '<div class="ff-rp-sec ff-rp-lockable"><span class="ff-rp-label" id="ff-rl-width">Width</span>' +
      seg("width", "ff-rl-width", [["narrow", "Narrow"], ["normal", "Normal"], ["wide", "Wide"]]) +
      '<p class="ff-rp-note">Enables on reading pages.</p></div>' +
      '<div class="ff-rp-foot"><p class="ff-rp-hint">Saved &amp; shared across Fairy Fox.</p>' +
      '<button type="button" class="ff-rp-reset" data-act="reset">Reset</button></div>';

    var range = panel.querySelector(".ff-range");

    function markActive() {
      panel.querySelectorAll("[data-act], .ff-swatch").forEach(function (b) {
        var act = b.getAttribute("data-act"), on = null;
        if (b.classList.contains("ff-swatch")) on = b.getAttribute("data-acc") === (prefs.accent || "");
        else if (act === "theme" || act === "lh" || act === "width") on = b.getAttribute("data-val") === prefs[act];
        if (on !== null) b.setAttribute("aria-pressed", on ? "true" : "false");
      });
      if (range) range.value = clampSize(prefs.size);
    }
    markActive();

    // Lock line spacing + width off a reading page: disable the controls (so they can't be
    // focused or clicked) and reveal the note. The saved values stay.
    if (!isReadable()) {
      panel.querySelectorAll(".ff-rp-lockable").forEach(function (sec) {
        sec.classList.add("is-locked");
        var g = sec.querySelector(".ff-seg");
        if (g) g.setAttribute("aria-disabled", "true");
        sec.querySelectorAll(".ff-seg button").forEach(function (b) { b.disabled = true; });
      });
    }

    panel.addEventListener("click", function (e) {
      var b = e.target.closest("button");
      if (!b || !panel.contains(b)) return;
      if (b.classList.contains("ff-swatch")) { prefs.accent = b.getAttribute("data-acc") || null; }
      else {
        var act = b.getAttribute("data-act");
        if (act === "close") { setOpen(false); btn.focus(); return; }
        if (act === "reset") { prefs = Object.assign({}, DEFAULTS); }
        else if (act === "theme" || act === "lh" || act === "width") prefs[act] = b.getAttribute("data-val");
        else return;
      }
      apply(); save(); markActive();
    });
    range.addEventListener("input", function () { prefs.size = clampSize(+range.value); apply(); save(); });

    function setOpen(open) {
      panel.classList.toggle("open", open);
      btn.setAttribute("aria-expanded", open ? "true" : "false");
      if (open) { var f = panel.querySelector(".ff-rp-close"); if (f) f.focus(); }
    }
    btn.addEventListener("click", function (e) { e.stopPropagation(); setOpen(!panel.classList.contains("open")); });
    document.addEventListener("click", function (e) {
      if (panel.classList.contains("open") && !panel.contains(e.target) && e.target !== btn) setOpen(false);
    });
    document.addEventListener("keydown", function (e) {
      if (e.key === "Escape" && panel.classList.contains("open")) { setOpen(false); btn.focus(); }
    });

    var wrap = document.querySelector(".site-header .wrap");
    var nav = wrap && wrap.querySelector(".nav");
    if (wrap && nav) nav.parentNode.insertBefore(btn, nav.nextSibling);
    else (wrap || document.body).appendChild(btn);
    document.body.appendChild(panel);
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
})();
