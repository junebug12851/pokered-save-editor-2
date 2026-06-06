# Session Log — pokered-save-editor-2

Chronological "what just happened" log, **newest first**. `../status.md` holds the *current*
state (health, open issues); this file is the running history of what changed each session and
why. Deep root-cause mechanics live in `../reference/qt6-patterns.md` and `../decisions/`; this is
the narrative.

The recurring theme of sessions 13f–13j: **QML garbage-collects parentless C++ QObjects.** Two
distinct fixes came out of it — `DB::qmlProtect` for DB entries (13f) and `qmlCppOwned()` for
savefile `Q_INVOKABLE` returns (13g–13h). See qt6-patterns "QML garbage-collects parentless C++
QObjects" for the full rule.

---

## Session 13z12 — Identity rename + Apache 2.0 tidy (no logic change)

Owner now goes by **Twilight** (not June / June Hanabi), and the personal gmail was removed.

- **Source headers (274 `.cpp`/`.h`/`.qml`):** `Copyright 2019/2020 June Hanabi` → `Copyright … Twilight`.
- **Bare "June" in QML/notes** (e.g. "June's call", "June confirmed") → "Twilight".
- **`boot.cpp`:** `organizationName` "June Hanabi" → "Twilight"; `organizationDomain`
  `pokeredsaveeditor.junehanabi.gmail.com` → `pokeredsaveeditor.twilight.app` (drops the gmail string).
  ⚠️ Changing org name/domain changes the **QSettings storage path** — existing local settings become
  orphaned. Acceptable for a WIP; revert both strings if you want old settings back.
- **6 files that lacked an Apache header** (`main.cpp`, `mainwindow.cpp/.h`, `types.h`,
  `savefile.cpp/.h`) now have the standard boilerplate (comment-only, no behavior change).
- **`credits.json`** project-leader name → "Twilight" (GitHub handle `junebug12851` kept).
- **`README.md`:** added a **License** section (Apache 2.0). **`savefile-structure.bt`** author → Twilight.
- **Kept as-is:** the `github.com/junebug12851` URLs/handle (still Twilight's account — links must work).
- **NOT touched:** git commit history still shows `June Hanabi <junehanabi@gmail.com>` as author, and no
  local `git config user.name/email` is set. Rewriting history / setting git identity is a separate,
  destructive step left for Twilight to decide.

LICENSE file itself was already correct, unmodified Apache 2.0 (201 lines).

---

## Session 13z11 — Trainer Card: compact field heights (QML-only, no rebuild)

- **Symptom (Twilight):** the trainer-card text boxes were too big/tall with too much padding and
  margin; rows didn't line up cleanly with their label text.
- **Cause:** the same Qt 6 Material control-height issue — `TextField`/`ComboBox` carry a tall
  implicit height, and each card field sized itself to that implicit height, so rows were tall and
  inconsistent.
- **Fix (documented "explicit height knob" pattern):** added `property int fieldH: 28` to
  `CardFront.qml` and applied it as an explicit `height` to PlayerIdEdit/MoneyEdit/CoinsEdit/
  StarterEdit, and as a new `fieldH` property on `PlaytimeEdit` (which forwards it to its
  Days/Hours/Minutes/Seconds/Frames sub-edits — all `DefTextEdit` roots, so `height:` applies
  directly). The wrappers' internal `height: child.implicitHeight` binding is overridden at the use
  site; inner controls `anchors.fill: parent` so they shrink. Labels are top/bottom-anchored +
  vcentered, so they track the shorter field and line up with the text. Also tightened the vertical
  rhythm (spacer→money 25→18; inter-row gaps 5→4).
- **Tuning:** single knob `CardFront.fieldH` (28) drives every field's height. Horizontal text
  padding inside the fields was left at `DefTextEdit`'s default (shared component) — can be trimmed
  per-instance on the card if Twilight wants the numbers tighter to their labels.
- **Follow-up (Twilight review of the clock):** two fixes in `PlaytimeEdit.qml`.
  (1) **Row was too tall / contents rode above center:** the `MouseArea` sized to
  `childRow.implicitHeight` (the Material ~48px implicit), so the 28px sub-fields sat top-aligned in
  a taller row → digits and the ":" looked high. Pinned the row to `top.fieldH` and vertically
  centered each `PlaytimeDivider` (`anchors.verticalCenter`). (Money/Coins were already fine — their
  wrappers got an explicit 28 height in the first pass, so they had no implicit-height row.)
  (2) **Fields too wide:** each digit field only needs ~2 chars, but Material's default horizontal
  padding bloated them (their `width = 2*font.pixelSize + leftPadding + rightPadding`). Added a
  `digitPad: 2` knob applied as `leftPadding`/`rightPadding` to every sub-edit, which both narrows
  the fields and shrinks the whole clock. All clock tuning now lives in `PlaytimeEdit.qml`
  (`fieldH`, `digitPad`).
- **Follow-up (ID/Money/Coins labels rode high):** the built-in label in `DefTextEdit.qml` is
  anchored `top`→`bottom` of the field but had only `horizontalAlignment` (no `verticalAlignment`),
  so its text defaulted to top-aligned and sat above center once the fields got shorter. Added
  `verticalAlignment: Text.AlignVCenter` to that Label. Shared component, but universally correct (a
  fill-height label should vcenter); Starter's own inline Label already had it.

## Session 13z10 — Restore Trainer Card centered box size (QML-only, no rebuild)

- **Symptom (Twilight):** the Trainer Card used to sit in a centered grey box (~middle of the window);
  after the large refactor it had grown to fill the whole window.
- **Cause:** in `screens/non-modal/TrainerCard.qml` the `SwipeView` (which holds `CardFront`, a
  bordered Rectangle that fills the view) was changed from a fixed centered size to
  `anchors.fill: parent` during the refactor. `CardFront` itself has no intrinsic size, so it tracks
  the SwipeView.
- **Fix:** restored the original `anchors.centerIn: parent` + `width: 500` + `height: 250` (values
  recovered from `git show HEAD:…/TrainerCard.qml`). Reverts back to the centered grey box.

## Session 13z9 — Filter order + "Normal Only" + notes sweep (QML-only, no rebuild)

- **Filter order** (`SearchCriteria.qml`): now All, Normal Only, Single-Char, Multi-Char, Variable,
  then a gap, then Picture, Control (the "special/dangerous" ones grouped at the bottom).
- **"Normal" → "Normal Only"**: Twilight's lightweight UX nudge — the word "Only" hints that leaving Normal
  (for Single-Char etc.) means leaving the always-safe set, reinforced by its ⓘ tooltip. No room/clutter
  cost; a one-word fix instead of a paragraph.
- **Notes comprehensively refreshed**: `status.md` (current state / open issues / runtime health rewritten
  to 13z9), `plans/next-steps.md` (full-keyboard target marked done, real next steps), `ui-patterns.md`
  (full-keyboard/quick-edit section rewritten to final state + new tuning knobs), `CLAUDE.md` (added the
  "new .qml → app.qrc" critical rule).

## Session 13z8 — Editor breakout bug-fixes (QML-only, no rebuild)

- **Dice icon squished**: AbstractButton scales the icon to `icon.width × icon.height` (no aspect
  preserve), and width≠height (14×15) stretched it. Set 16×16 on both dice buttons.
- **Full-editor preview stayed box-shaped after Example→Name**: the footer `ColumnLayout` was
  overriding the `NameDisplay`'s own `width`/`height` bindings, so it couldn't shrink. Re-anchored the
  footer (toggle row `anchors.top`, preview below it) so the NameDisplay sizes itself again. Same change
  fixed the **too-much-margin above the Name button** (toggle row now `topMargin: 6`).
- **Quick edit showed the OLD name in the field (preview was correct) after editing in the full
  keyboard**: a `TextField`'s `text:` binding breaks on first keystroke, so it went stale. Dropped
  `text: str` and seed `popupEdit.text = img.str` in `onEditorVisibleChanged` (before open); the field
  still pushes up via `onTextChanged`.

## Session 13z7 — Quick-edit broken out of the ⋮ menu (REBUILD: 2 new qrc files)

Twilight: menus were clean UI but bad UX (too much clicking). Broke the editors' overflow menus into
explicit controls.

- **New `general/FlatToggle.qml`** — extracted the flat square toggle from `NameFullHeader` (was an
  inline component) so it's reusable. **New `general/SimulatedTilesetCombo.qml`** — moved the tileset
  combo out of `name-full/NameFullTileset` into `general` (reusable + avoids a general→name-full import
  cycle). **Both added to `app.qrc` → a rebuild is required** (new files don't hot-reload).
- **⋮ menus gone.** `NameEdit` and `NameFullEdit` dropped the ellipsis menu; in its place a **dice
  Randomize-Name button**. `NameEdit` lost its `changeStr/toggleExample/reUpdateExample` signals and
  `hasBox`; `disableMenu` → `disableRandomize`. (`NameDisplayMenu`/`NameDisplayMenuNoTileset`/
  `TilesetMenu` are now unused — left in place, not instantiated.)
- **Name/Example toggle** (FlatToggle, "Name" default) + a **next `>>` button** to re-roll the example:
  quick-edit popup upper-right, full editor above the footer preview. Drives the local example state
  (popup → `img.toggleExample`/`reUpdateExample`; keyboard → `FullKeyboard.toggleExample`/…).
- **Simulated bar at top of the popup** — "Simulated" label + Outdoor toggle + tileset combo, replacing
  the old "Simulated Tileset" submenu. Popup widened to 430; the "Edit Name" title was dropped.
- Example still **defaults off** (popup resets on close; keyboard `hasBox` defaults false).

`NameEdit`/`NameDisplay` are shared (player/rival/nickname + keyboard footer) — verify all on the rebuild.

## Session 13z6 — Example demo decoupled from the row (QML-only, no rebuild)

The "example" (textbox demo) used to be the row image's `hasBox`, shared via `img` and propagated to
the full keyboard by binding + a toggle callback — so toggling an example in an editor flipped the
**regular** name display (trainer card / rival / Pokémon) into a box. Twilight: the regular page must only
ever show the name; examples belong only inside the editors, and the two editors shouldn't share a
global.

- **`NameDisplay`**: added local `popupExample` + `popupPlaceholder`. `toggleExample()` /
  `reUpdateExample()` now drive those (not the row's `hasBox`). The quick-edit popup preview renders
  its OWN box source when `popupExample` is on; the row image is unaffected (always name-only).
  `popupEdit.hasBox` → `img.popupExample`; popup resets `popupExample` on close. Removed the
  keyboard→row example callbacks and stopped passing `hasBox`/`placeholder` into the keyboard.
- **`FullKeyboard`**: owns its example locally — `toggleExample()`/`reUpdateExample()` are now
  functions on the keyboard (were signals that round-tripped to the row); footer preview uses the
  keyboard's own `hasBox`/`placeholder`. `preClose` stays a signal (commit hook).
- **`NameDisplayMenu`** (quick-edit popup): un-commented **Toggle Example** / **Randomize Example**
  (the keyboard menu already had them).

`NameDisplay` is shared by player / rival / nickname (and is the keyboard footer) — verify all on a run.

## Session 13z5 — Image-only pill tooltips; combo widen; Normal tooltip trim (QML-only)

- **Pill hover tooltips are now image-only** — removed the description `Label` entirely (Variable /
  Multi-Char / Picture characters were still showing `tip` text). Just the rendered tile. (The side
  `DetailView` still shows text on hover; only the popup tooltip changed.)
- **Tileset combo** widened ~4 chars (`preferredWidth` 104 → 132).
- **Normal filter ⓘ tooltip** trimmed back to just its own description (dropped the non-normal caveat) —
  Twilight's call, cleaner; she'll find another home for that note later.

## Session 13z4 — "All" filter radio (QML-only, no rebuild)

Added an **"All"** radio at the top of the filter list (in the same `ButtonGroup`). When selected,
`SearchContainer.reSearch` calls `startOver()` (whole store) instead of `keepAnyOf`, so it shows
everything — including any entry with no category flag. Default selection stays **Normal**.

## Session 13z3 — Filters → radios; Simulated group narrowed (QML-only, no rebuild)

- **Filters are now single-select radio buttons** (`SearchParam` CheckBox → RadioButton) in a shared
  `ButtonGroup` (needed because the radios live in separate rows, so auto-exclusive wouldn't group
  them). One category is always active → **Clear button removed**. `keepAnyOf` is unchanged (union of
  the one selected = that category), so the backend still works and multi-select could return.
- **Simulated group narrowed again**: `FlatToggle` horizontal padding 9 → 5, group spacing 6 → 3,
  tileset combo `preferredWidth` 124 → 104 (vertical padding kept at 9 — Twilight wanted it taller).

## Session 13z2 — Full keyboard nits (QML-only, no rebuild)

- **Simulated group condensed**: cut horizontal padding/margins ~50% (`FlatToggle` left/right 18 → 9,
  group spacing 12 → 6, combo `preferredWidth` 150 → 124) and bumped vertical padding (7 → 9). The whole
  group was far too wide.
- **Normal tooltip trimmed**: dropped the "this simple simulation…" tail — implied, and it cluttered.
- **Tileset last tile (bold "9") now clickable/hoverable**: off-by-one — `fontAt` is 1-based so valid
  indices are `1..fontCount()` inclusive; `TilesetPicker`'s `>= fontCount()` dropped the last tile.
  Changed to `> fontCount()`. See `reference/fix-patterns.md`.

## Session 13z — Full keyboar