# Project Status

_Last updated: Session 13z11 (2026-06-05)_

This file is the **current state** only. For the chronological history of what changed each
session and why, see `sessions/session-log.md`. For root-cause mechanics, see
`reference/qt6-patterns.md` and `decisions/`.

## Current State (read this first)

The big structural blocker is **solved**: the `brg.file.data.dataExpanded.*` chain works, data
reads and **persists** across every screen, and the build is fast. The other major class of
bugs — **QML garbage-collecting parentless C++ QObjects** (which caused the font/name blanking and
the clicking-Pokémon crash) — is also fixed (DB entries via `DB::qmlProtect`; savefile
`Q_INVOKABLE` returns via `qmlCppOwned`). Twilight confirmed: data flows, names render, no crash when
clicking around.

We are in a **UI-polish phase**. Two big screens are now polished + Twilight-iterated:

1. **Pokémon details editor** (General / DV-EV / Moves tabs + Glance pane) — sessions 13k–13t, Twilight
   signed off ("looking solid"). Conventions in `reference/ui-patterns.md`.
2. **Name editors — full keyboard + quick-edit popup** (`name-full/*`, `general/NameDisplay.qml`) —
   sessions 13v–13z8, heavily iterated with Twilight live. Now: wide centered name box; a **"Simulated"**
   group (label + Outdoor toggle + tileset combo + **Grid/Tileset** view toggle) using a flat-square
   `FlatToggle`; **single-select radio filters** (All / Normal Only / Single / Multi / Variable /
   Picture / Control) with help on a ⓘ dot; a **color-coded pill grid** whose hover tooltip renders the
   actual tile (image only, static, control = none); the **⋮ menus removed** in favor of explicit
   buttons (dice Randomize-Name, a **Name/Example** toggle + `>>` re-roll); the popup gained a Simulated
   bar across its top. The **example/box demo is now local to each editor** (never the regular name row).

The recurring underlying theme remains the **Qt 6 Material control-height issue** (Qt 6.5+ taller
`TextField`/`ComboBox`); the fix everywhere is proper layouts, not pixel offsets
(`reference/qt6-patterns.md`). **Read `reference/ui-patterns.md` before more UI work.**

**Last sessions (s13z10–13z11) — Trainer Card layout pass (QML-only, no rebuild):** (1) restored the
**centered grey box** (the `SwipeView` had been changed to `anchors.fill: parent` in the refactor →
back to centered `500×250`); (2) **compacted every field** via one `CardFront.fieldH` (28) knob +
tighter row margins, fixing the too-tall Qt 6 Material boxes; (3) **fixed the playtime clock** —
narrowed each digit field to ~2 chars (`PlaytimeEdit.digitPad`) and stopped the row sizing to the
Material implicit height (it now pins to `fieldH` and vcenters the `:`), which had made the digits ride
high; (4) **vcentered `DefTextEdit`'s built-in label** (it lacked `verticalAlignment`, so ID/Money/Coins
labels rode high). All Twilight-driven, live-iterated.

**Next:** continued Twilight review of the name editors; an end-to-end save/reopen verification pass. See
`plans/next-