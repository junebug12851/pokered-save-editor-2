# UI Patterns & Conventions

How this app's QML UI is built and styled, distilled from the sessions-13k–13t polish pass on the
Pokémon details editor. **Read this before doing UI work** so screens stay consistent. Twilight's
standing rule (see `../context/principles.md` → "The Quality Bar"): **proper layouts, no
fixed/negative-margin hacks.** When a "things overlap / wrong size" bug appears, the cause is almost
always the Qt 6 Material control-height change (`qt6-patterns.md` → "Material 3 control heights") and
the fix is a real layout, not a pixel offset.

Deep mechanics live in `qt6-patterns.md` and `fix-patterns.md`; this file is the "how we do UI here"
playbook.

---

## Labeled field rows ("option #2")

The standard "label : control" row. Twilight's chosen look: the shaded label box grows to the field's
height so label-strip and field read as one aligned row.

```qml
// Reusable shaded label cell (see OverviewTab.qml's inline `component FieldLabel`)
component FieldLabel: Rectangle {
  property alias text: labelText.text
  Layout.preferredWidth: 110
  Layout.fillHeight: true                 // grows to the row's field height
  color: Qt.lighter(brg.settings.textColorMid, 1.75)
  HeaderText { id: labelText }            // HeaderText fills + right-aligns + vcenters
}

RowLayout {
  Layout.fillWidth: true
  spacing: 8
  FieldLabel { text: "OT Name" }
  DefTextEdit { Layout.alignment: Qt.AlignVCenter; Layout.preferredHeight: top.textH; … }
  Item { Layout.fillWidth: true }         // ← spacer BEFORE any ⋮ so the ⋮ right-aligns
  IconButtonSquare { … }                  // optional trailing menu
}
```

Rules:
- Control is `Layout.alignment: Qt.AlignVCenter`; **never** `anchors.top + fixed/negative topMargin`.
- A trailing `Item { Layout.fillWidth: true }` goes **before** the ⋮ button so the dots pin to the
  row's right edge regardless of field width (a trailing spacer *after* the ⋮ does NOT right-align it).
- For a fill control (a slider), give it `Layout.fillWidth: true` instead of the spacer.

## Field heights (consistency)

Rows felt "oddly sized" when each sized to its control's natural height (Material combos and text
fields differ). Give the screen explicit height knobs, one per control type, so rows are consistent:

```qml
property int textH: 30    // text boxes (shorter)
property int comboH: 38   // combos (a touch taller)
// each control: Layout.preferredHeight: top.textH  (or top.comboH)
```
`DefTextEdit` sets `verticalAlignment: AlignVCenter` + `topPadding:0; bottomPadding:0` so its text
stays centered at any height.

## Borderless combo boxes with hover underline

Combos are borderless at rest (clean) but must signal interactivity on hover. Pattern used by all
`Select*` combos:

```qml
ComboBox {
  id: control
  flat: true
  property color hoverColor: brg.settings.accentColor   // blue-ish default
  background: Rectangle {
    color: "transparent"
    Rectangle {                                          // hover underline
      anchors.bottom: parent.bottom; width: parent.width; height: 2
      visible: control.hovered
      color: control.hoverColor
    }
  }
}
```
- **Underline color by context:** default `accentColor` (blue-ish) on normal backgrounds; on
  **white-text header/accent bars** override `hoverColor: brg.settings.textColorLight` at the
  instance (e.g. `SelectSpecies` / `SelectStatus` in `GlancePane`). Do NOT use `primaryColor` — it's
  red and Twilight rejected it for underlines.
- The custom combo `popup` must cap its height or it can't scroll — see `qt6-patterns.md`
  → "Custom ComboBox popup must cap its height".

## ⋮ icon menu buttons — `IconButtonSquare`

`IconButtonSquare` is the standard flat icon button. It bakes a **tight, rectangular hover/press
highlight** (`padding: 6`, `radius: 2`) instead of Material's wide rounded button. **Use it as-is** —
don't re-add `padding:0`, custom backgrounds, or `Layout.preferredWidth/Height` per instance (those
caused the inconsistent / too-wide / stretched ⋮ we cleaned up). Per-instance you only set
`icon.width: 7`, `icon.source`, `icon.color`.

- The ⋮ glyph (`ellipsis-v.svg`) is **tall and narrow** — it aspect-fits, so setting only
  `icon.width` shrinks it to a sliver. The whole app uses `icon.width: 7` (with the default
  `icon.height: 15`); keep that for consistency. Don't "widen the button" to enlarge the dots — it
  doesn't, and over-sizing the icon stretches/oversizes them. (See `fix-patterns.md`.)
- The Bag screen's header/footer icon buttons used to repeat `leftPadding:0; rightPadding:0;
  leftInset:0; rightInset:0` on every instance — exactly the anti-pattern above. Cleaned up: they're
  now bare `IconButtonSquare`s in a centered `RowLayout` (header spacing 12, footer spacing 15). A
  reminder comment lives in `IconButtonSquare.qml` itself.

## Bag / Items screen layout (the standard for this screen)

`screens/non-modal/Bag.qml` → two `ItemsPane` in a **`RowLayout`**, each `Layout.fillWidth` +
`Layout.fillHeight` (50/50 split, no `Math.trunc(width*0.5)` math). Each `ItemsPane`
(`fragments/screens/bag/`) is a `Rectangle` with an anchored 45px **header bar** (check-all
`IconButtonSquare` parked at the bar's left edge, title `Text` centered in the bar with the
`(count/max)` `Text` anchored to its right) at top, an anchored 45px
**footer bar** (centered `RowLayout` of the bulk-action `IconButtonSquare`s, each `visible:
model.hasChecked`) at bottom, and an `ItemBoxView` filling between them (15px left/right inset). No
magic-width wrapper boxes. `ItemBoxView` is a `ListView`; rows are a **left-aligned** `RowLayout` [CheckBox |
SelectItem | DefTextEdit count] (`Layout.alignment: Qt.AlignLeft`) so each row's checkbox forms a
column directly under the header's check-all button — both share the list's 15px left inset. Pinned to
the `rowH`/`comboH`/`textH` knobs so the three differently-sized Material controls vcenter and line up. The "+" add row is the placeholder delegate; bottom
breathing room is a `footer: Item { height: 25 }` (not an empty trailing `Text`).

## Pokémon storage screen layout (the standard for this screen)

`screens/non-modal/Pokemon.qml` mirrors Bag: two `PokemonPane` in a **`RowLayout`**
(`spacing: 0`), each `Layout.fillWidth` + `Layout.fillHeight` for a 50/50 split (was
`width: Math.trunc(parent.width * 0.50)` + chained anchors). Each `PokemonPane`
(`fragments/screens/pokemon/`) is a `Rectangle` with an anchored 45px **header bar**
(check-all `IconButtonSquare` parked at the bar's left edge `leftMargin: 24`; a
`SelectPokemonBox` switcher `anchors.centerIn`; the "set as current box" dot
`IconButtonSquare` anchored to the switcher's right, `visible` only when this pane
isn't the current box) at top, and a `PokemonBoxView` filling the rest (15px
left/right inset, 15px bottom margin). Removed the old `width: 265` magic-width wrapper
`Row` and every repeated `leftPadding/rightPadding/leftInset/rightInset: 0` override on
the `IconButtonSquare`s (use them bare per "⋮ icon menu buttons" above). Note: when the
dot button was briefly nested *inside* the combo, `model` rebound to the combo's model —
it must reference `top.model.curBox`; as a sibling it's clearer.

**No footer bulk-action bar (removed once drag & drop landed).** The old 45px footer of
move-to-top/up/down/bottom + transfer + release `IconButtonSquare`s (each `visible:
model.hasChecked`) is **gone** — reorder and cross-pane moves are now drag gestures, and
delete moved onto the cell (below). The model's `checkedMove*`/`checkedTransfer` slots
still exist but are now unused by the UI; `checkedToggleAll` (header check-all) and
`checkedDelete` (group delete) are still used.

**Checkbox selection — scoped persistence (Twilight's exact rule).** Selection should survive
**only** the Pokémon-detail editor round-trip (open a mon → back), and should **clear** on a box
switch and on leaving the screen (back / Home). The mechanics:
- The delegate `CheckBox` **binds** `checked: (itemChecked === true)` and writes back only
  `onToggled` (the old one-shot `Component.onCompleted: checked = itemChecked` didn't restore on
  delegate reuse/reset → "checks disappear"). Checked state is a per-mon QObject property, so it
  survives a model reset — which is what makes the editor round-trip restore work (closing the
  editor only resets the model; the page itself stays mounted).
- **Box switch clears** (`PokemonStorageModel::switchBox` clears the outgoing box's checks).
- **Leaving the screen clears** via `Pokemon.qml`'s `Component.onDestruction` →
  `pokemonStorageModel1/2.clearCheckedState()`. This is the key trick: `appBody` is a `StackView`,
  and opening the editor **pushes** `PokemonDetails` *over* `Pokemon.qml` (which stays alive →
  `onDestruction` does NOT fire → selection kept); leaving the screen **pops** `Pokemon.qml`
  (destroyed → fires → cleared). The router's `closeNonModal` can't distinguish the two (it fires
  for both editor-close and screen-close), so we deliberately drive the clear off page destruction,
  not a router signal. (`PokemonStorageModel::pageClosing` is now an inert hook.)
- Transfers/deletes still clear the specific mons they touch; `hasChecked` recomputes per box via
  `curBoxChanged → onReset → hasCheckedChanged → checkStateDirty`.

**Whole-cell hover via a `HoverHandler`** (`cellHover`), NOT `dragHandler.containsMouse`: the
checkbox and delete button key their visibility off `cellHover.hovered`. A child `Button`/
`CheckBox` that's hovered **steals** hover from the cell `MouseArea`, so `containsMouse` would
flip false the moment you reach for the delete button and it'd vanish — the `HoverHandler`
stays true over the whole cell incl. its child controls.

**Per-cell delete button** (`deleteBtn`, a round `Button` chip in the cell `content`):
bottom-right (5px margins), `visible: !itemIsPlaceholder && (cellHover.hovered || itemChecked)`.
A real button with states: at rest an **opaque accent chip** (`accentColor`, the in-screen
"menu bar" colour — a semi-transparent rest circle looked bad, and since it only shows on
hover/checked an opaque chip is fine) with a **white** (`textColorLight`) `times.svg`; on hover the
chip fills `primaryColor` (the X stays white); on `down` it darkens (`Qt.darker(primaryColor,1.25)`);
`Behavior on color` for a 90ms fade. **`24×24`** chip. **`icon 16×23`** — the `times.svg` viewBox is
`352×512` (taller than wide, heavy vertical padding), so keep `width ≈ 0.69·height` for a square,
un-stretched visible X; the padding lets the tall icon box still fit the circle. **`icon.width/height`
are `int` — a non-integer (e.g. `13.75`) is a hard QML type error that fails the whole component and
the screen won't open; keep them whole numbers.** `radius width/2`, all insets/padding 0, `z: 100`.
`onClicked: theModel.deleteMon(index, itemChecked)` — a **checked** mon deletes the whole
checked set (`deleteMon` `group` → `checkedDelete()`); otherwise just that mon (single path keeps
the party non-empty and reveals the trailing "+" slot if the box was full).

`PokemonBoxView` cell (`GridView` delegate, `cellSize: 100`): species/shiny `Image`
anchored top→`nameLabel.top` (margins 8) so the icon and name stack as one unit; a level
badge pill top-right; a hover `CheckBox` top-left; and an **always-visible name label**
(`Text`, `getMonNickname()` → nickname or species fallback) anchored across the bottom,
**`textColorDark` on no background**, `pixelSize 12`, `AlignHCenter`, `elide: ElideRight`.
The old hover-only accent **edit pill + pen icon was removed** (the cell-wide `MouseArea`
already opens the editor on click, so the pen/button was redundant); with it went the
`QtQuick.Effects` import and the pen `MultiEffect` tint. Names show **always** here now —
contrast the trainer/rival/Pokémon *name-row* convention where names show on hover.

### Drag & drop reordering + cross-pane transfer (the standard for this grid)

`PokemonBoxView` cells support **drag-to-reorder within a pane** and **drag-to-transfer
between the two panes**. Twilight's chosen interaction (decided up front): **insert at the
drop slot**, **drop-to-commit** (no live reshuffle — most reliable on a C++
`QAbstractListModel`-backed `GridView`), a **drag threshold** so a plain click still opens the
editor, **group operations via the existing checkboxes**, and a **dashed placeholder** marking
the hovered drop slot.

How it's built (`PokemonBoxView.qml` delegate):

- The **delegate root is a `DropArea`** (`id: cell`), sized to the cell. It exposes the info a
  drop target needs off the dragged item: `ownerModel` (`view.theModel`), `cellIndex` (`index`),
  `grabbedChecked` (`itemChecked === true`), `isPlaceholder`.
- The visible cell is a child **`content` `Item`** (centered via center anchors). It's the **drag
  target** (`Drag.source: cell`, hot-spot centered). While dragging, a `State { when:
  content.Drag.active }` **reparents `content` to `view.dragLayer`** (`property Item dragLayer:
  Overlay.overlay`) and clears its center anchors, so the "ghost" floats **across both panes** and
  isn't clipped by the `GridView`. The source slot empties as the content lifts (free "picked up" feel).
- **You must drive `Drag.active` manually and call `Drag.drop()` on release** (don't bind
  `Drag.active` to the MouseArea's `drag.active`). An *internal* MouseArea drag never auto-commits, so
  `DropArea.onDropped` will **never fire** on its own — silently no-op drops. Pattern: track
  `property bool maActive: dragHandler.drag.active`; in `onMaActiveChanged`, set `content.Drag.active =
  true` when it goes true, and when it goes false call `content.Drag.drop()` (fires `onDropped` under
  the cursor) **then** set `content.Drag.active = false` (reverts the lift). This was the cause of the
  first cut's "drag does nothing, no error" bug.
- The drag handler is the cell's `MouseArea` (`dragHandler`): `drag.target: cell.isPlaceholder ?
  null : content` (the "+" slot isn't draggable), `drag.threshold: 8`, **`preventStealing: true`**
  (so the `GridView` flick can't steal the gesture). Its `onClicked` is the **unchanged**
  open-editor path — a press that moves less than the threshold stays a click; a completed drag
  suppresses the click automatically.
- **Drop target slot = the hovered cell** (insert *before* it). The trailing "+" placeholder cell's
  `index == count`, so dropping there appends. `onDropped` reads `drop.source.*`, then dispatches:
  same pane → `theModel.dragReorder(from, to, group)`, other pane → `srcModel.dragTransfer(from,
  to, group)`. `group` = the grabbed mon was checked (then the whole checked set moves).
- **Defer the model mutation with `Qt.callLater`** inside `onDropped`. The mutation resets the model
  (rebuilding these delegates); running it next tick lets the dragged `content` reparent back to its
  cell first, so no delegate is destroyed while it still owns the floating ghost (avoids an orphaned
  ghost / dangling visual).
- The **drop indicator is an insertion caret** (Twilight's call): a `Canvas` (`dropHint`) drawing a
  **dashed vertical bar straddling the cell's LEFT edge** (`width: 6`, `anchors.left` + `leftMargin:
  -3` to center on the cell boundary, `lineWidth 3`, `setLineDash([5,4])`, round caps),
  `visible: cell.containsDrag`. It marks the **gap before** the hovered cell — i.e. *between* entries —
  and is a pure overlay, so **icons never shuffle or resize** while dragging (Twilight explicitly
  rejected the earlier full-cell box that sat *over* a mon and the idea of live-reflowing entries — it
  made the user fight a moving target, esp. at row ends). Hovering the trailing **"+" slot** puts the
  caret at its left edge = the gap **after the last mon, before the New button** (so "+" stays a valid
  drop-at-end target, just with a between-entries caret instead of a box).

Backing C++ (`PokemonStorageModel`): two `Q_INVOKABLE`s — `dragReorder(from, to, group)` (in-box
splice + `onReset()`; count unchanged so no `pokemonChanged` needed) and `dragTransfer(from, to,
group)` (mirrors `checkedTransfer`'s party↔box conversion + capacity / last-party-mon guards via
`relocateOne`, then slides the appended block to the drop slot). `group` pulls the set from
`getChecked()`. `toIndex` is the destination insertion slot (0..count; insert before, == append at
count). These are the drag analogue of the `checked*` bulk actions.

## Centered overlay editor popups (escape clipping)

An editor that opens near a screen edge gets clipped by ancestors (tabs/headers). Don't anchor it
outside the parent — render it in the window overlay as a centered, dismissible popup. The
`NameDisplay` Stage-2 editor uses this (shared by player-name / rival / nickname):

```qml
Popup {
  parent: Overlay.overlay
  anchors.centerIn: Overlay.overlay
  modal: true; dim: true; focus: true
  closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
  onClosed: editorVisible = false
  // background Rectangle (radius 10), ColumnLayout: title, live preview, field, feedback
}
// drive open/close: onEditorVisibleChanged: editorVisible ? popup.open() : popup.close()
```
- Put the field + its action buttons in a **self-contained `RowLayout`** (in-flow), not anchored
  outside the field — otherwise the popup can't size to them. `NameEdit` is now such a RowLayout
  (`field Layout.fillWidth` + menu/accept/keyboard buttons). The popup then needs no guessed width.
- **`NameEdit` is dual-use — do not break the field-only use.** Besides this popup, `NameFullEdit`
  (the full-keyboard input) instantiates `NameEdit` with all buttons disabled and sets text-field
  properties on it (`topInset`, `width`, `selectedColor`). So `NameEdit` must keep exposing those
  (e.g. `property alias topInset: txtField.topInset`). Changing its root type once broke the whole
  full keyboard (s13u). Check `NameFullEdit` + `NameDisplay` before editing `NameEdit`.
- A **modal popup blocks other modals.** If a button inside opens another stacked screen (e.g. the
  full keyboard via `appRoot.push`), **close this popup first** (`editorVisible = false`) or the new
  screen is stuck behind the dim.
- Live in-game name preview: a plain `Image { source: img.source }` reuses the row image's animated
  provider source (no recursion).

## Scrollable forms (reserve scrollbar room)

A `ScrollView` + `ColumnLayout` form. The Material scrollbar is an **overlay** at the right edge, so
right-aligned controls (⋮ buttons) end up under it and become unclickable. Reserve room:

```qml
ScrollView { id: sv; anchors.fill: parent; clip: true; contentWidth: availableWidth
  ColumnLayout { width: sv.availableWidth - 16; … } }   // -16 keeps ⋮ clear of the scrollbar
```

## Sliders: value tooltip on hover + quick fade

Sliders without a separate readout (HP, EV) show their value in a tooltip on **hover and press**,
with a fast fade (Material's default felt slow):

```qml
ToolTip {
  parent: slider.handle
  visible: slider.pressed || slider.hovered
  text: slider.value.toFixed(0)
  Material.background: brg.settings.accentColor
  Material.foreground: brg.settings.textColorLight
  enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 70 } }
  exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 70 } }
}
```

## Disabled / "coming soon" Home tiles

The Home grid (`HomeIconsModel.qml` → `IconsView` → `IconDelegate.qml`) can grey out a tile for a
screen that isn't available yet. The model carries a `disabled` bool role per element; the delegate
does two things with it:

- `enabled: !model.disabled` — proper non-clickability (no hover, no press ripple, no navigation).
- a `MultiEffect` layer (`saturation: -0.6`, `brightness: -0.15`) when `model.disabled` — a *slight*
  desaturate + darken so the tile reads as muted/unavailable rather than fully faded. Same
  `QtQuick.Effects` / `layer.effect` pattern the Pokédex seen/owned indicators use. Tune the two
  values to taste; they're the only dials.

Currently disabled: **Maps, Options, Hall of Fame, Event Pokemon** (the latter three have no `page`
yet; Maps has a `page` but is held back). Flip a tile back on by setting its `disabled: false`.

## Material controls fight small heights

`CheckBox` / `Button` have a ~40px implicit/min height (touch target) that floors a layout row even
when you set `Layout.preferredHeight`. To actually shrink a row: `Layout.minimumHeight: 0` on the
control (+ optionally `Layout.maximumHeight` on the row). See `fix-patterns.md`.

## Centering one item with a sibling beside it

To center item A in a row while a small B sits beside it *without* shifting A off-center (e.g. the
DV/EV toggle + its ⋮): use an `Item` with A `anchors.horizontalCenter: parent.horizontalCenter` and
B `anchors.left: A.right; anchors.verticalCenter: A.verticalCenter`. A centered RowLayout of `[A,B]`
centers the *pair*, leaving A left-of-center.

## Full keyboard + quick-edit patterns (s13v–s13z8)

The font editors — the modal full keyboard (`name-full/*`, `screens/modal/FullKeyboard.qml`) and the
quick-edit popup (`general/NameDisplay.qml`) — were rebuilt across sessions 13v–13z8. Final conventions:

- **Pill grid for pickable items** (`SearchResults.qml`): a `Flickable > Flow > Repeater` of
  fixed-height (`22`), variable-width rounded `Rectangle` "pills", `Flow.spacing: 2`. Color each pill by
  category via a `determineColor(ind)` helper — bg `Qt.lighter(cat, 1.88)` at rest / `1.55` hover,
  border brightens to `cat` on hover, label `Qt.darker(cat, 1.25)`, font 12. `required property int
  fontInd` reads the model role. **`Repeater` instantiates ALL delegates eagerly** — keep per-delegate
  cost low.
- **Hover tile tooltip — image only** (`TilePreview.qml`): shows what a code looks like in-game (one
  byte can expand to several rendered chars, e.g. a Variable → a name). Reuse the `image://font`
  provider with a tight `chop` (`countSizeOfExpanded`, clamped 20), `no-box/1-line`, on a **white**
  rectangle (GB glyph pixels are dark). **No description text** in the tooltip (s13z5). **`TilePreview`
  is STATIC** (no animation timer, s13y) — animating an expensive expand per frame froze hovering. The
  preview is built by a `Loader { active: tip.opened }` so only the ONE hovered pill instantiates it
  (s13x — a child of a *closed* popup still reports `visible:true`, so eager timers pegged the CPU; see
  `qt6-patterns.md`). **Control pills get no tooltip at all.**
- **Single-select radio filters** (`SearchCriteria.qml`, `SearchParam.qml`, `SearchContainer.qml`):
  `RadioButton`s in a shared `ButtonGroup` (auto-exclusive only works among one parent's siblings, and
  these live in separate rows). Order: **All, Normal Only, Single-Char, Multi-Char, Variable** then a
  gap then **Picture, Control**. Backend = `FontSearch::keepAnyOf(...)` (union; one selected = that
  category); **All** calls `startOver()`. No Clear button (one is always active). "Normal **Only**"
  subtly signals that leaving it leaves the always-safe set. *(History: started tristate → AND s13v →
  OR/union s13y → radios s13z3; Twilight's call each time.)*
- **Compact filter rows**: Material `CheckBox`/`RadioButton` floor at ~40px — trim `topPadding`/
  `bottomPadding` + `Layout.minimumHeight: 0` to halve the spacing.
- **Help on a ⓘ dot, not the whole row** (`SearchCriteria.qml` `HelpDot`): a `Label "ⓘ"` with its own
  `HoverHandler` driving a `MainToolTip` (default placement). Hovering the checkbox/row should NOT
  trigger it. **Reserve scrollbar width** in the panel (`SearchContainer` `criteria.width = topz.width
  - 16`) so the right-aligned dots aren't under the scrollbar.
- **`FlatToggle`** (`general/FlatToggle.qml`): the flat, **square** (radius 0), **no-shadow**
  (`Material.elevation: 0`, custom background) toggle button — filled accent when `active`, outlined
  otherwise. Used for Outdoor, Grid/Tileset (keyboard), and Name/Example (both editors). Twilight rejected
  Material's elevated/rounded buttons here.
- **Paged view toggle, not swipe** (`PagedPicker`): Twilight dislikes `SwipeView` dots/gestures (clip over
  content). Keep `SwipeView { interactive: false }` for the slide, drive `currentIndex` from a header
  `FlatToggle` (`showTileset`) whose label reflects the **current** view ("Grid" / "Tileset").
- **`SimulatedTilesetCombo`** (`general/`): the app-wide tileset picker (drives
  `brg.settings.previewTileset`). In `general` so the keyboard header AND the popup reuse it (was
  `name-full/NameFullTileset` — moved to avoid a `general → name-full` import cycle).
- **Editors are menu-free** (s13z7): Twilight's rule — *UI isn't first, UX is*; the old ⋮ overflow menu
  meant too much clicking. `NameEdit`/`NameFullEdit` dropped the menu for a **dice Randomize-Name
  button** (square the icon: `icon.width == icon.height`, the button stretches non-square icons).
  Example actions are a **Name/Example `FlatToggle` + a `>>` (angle-double-right) "next" button** that
  re-rolls the example — popup upper-right, full editor above the footer preview. The quick-edit popup
  also hosts the **Simulated bar** (label + Outdoor + tileset combo) across its top.
- **The "example" (box demo) is LOCAL to each editor, never the row** (s13z6): the regular name display
  (trainer card / rival / Pokémon) shows only the name. The popup has its own `popupExample` +
  `popupPlaceholder` (its preview builds its own box source); the keyboard owns its `hasBox` +
  placeholder. Defaults off (popup resets on close). Do NOT route example state through the row's
  `hasBox` or an app-wide global.
- **Footer/anchor gotcha**: don't put the footer `NameDisplay` preview in a `ColumnLayout` — the layout
  overrides its own `width`/`height` bindings and the box→name toggle stays box-shaped/distorted. Use
  anchors (s13z8).
- **Re-seed a popup field on open** (s13z8): a `TextField`'s `text:` binding breaks on the first
  keystroke, so it goes stale after an edit elsewhere (e.g. the keyboard). Set `field.text = source` in
  `onEditorVisibleChanged` (before `open()`) and let it push up via `onTextChanged`.

## Commit edits on FINISH when the write is expensive or byte-touching

If a field's model write is cheap (a plain `MEMBER` setter, e.g. Pokémon nickname), binding
`onStrChanged`/`onTextChanged → model` per keystroke is fine. But if the write does real work or
touches save bytes — the **player name/ID** write cascades OT data across every stored Pokémon — do
**not** write per keystroke:

- It hangs (full storage rescan every character) and storms the two-way bind.
- It can be *wrong*: an intermediate typed value can momentarily match other data and corrupt it (a
  partial player name matching a traded mon's OT → that mon gets swept in). See
  `player-name-hang.md`.

Pattern: keep the live `str`/`text` for the preview, but persist the model **once, on finish**.
`NameDisplay` exposes a `committed(string val)` signal (fires when its popup or the full keyboard
closes); consumers use `onCommitted`. Plain `TextField`s use `onEditingFinished`. Editors here are
modal, so the user can't save mid-edit — commit-on-close always lands before any save. Always pair the
model setter with an equality guard (`if(val == cur) return;`) on the C++ side so redundant commits and
the bind's round-trip are no-ops.

Note on shared components: a base `.qml`'s own `onX` handler and a using file's `onX` for the same
signal **both run** (the derived one doesn't replace the base) — that's why `NameDisplay` can keep its
internal `onStrChanged` recalc while a consumer adds its own handler.

---

## Editor tuning knobs (where the dials are)

| Want to change | Knob |
|---|---|
| General-tab field heights | `OverviewTab.qml` `textH` / `comboH` |
| Items/bag row heights (checkbox/combo/count) | `ItemBoxView.qml` `rowH` / `comboH` / `textH` |
| Row gaps (General tab) | `OverviewTab.qml` ColumnLayout `spacing` / RowLayout `spacing` |
| Move pill height | `PokemonMoveSel.qml` `rowH` |
| DV/EV slider height & row gaps | `DvStatGroup`/`EvStatGroup` slider `Layout.preferredHeight`, grid `rowSpacing` |
| Future-Shiny row height | `StatsTab.qml` `futureShinyRow` height |
| Combo hover underline color | `Select*` `hoverColor` (default accent; header instances → `textColorLight`) |
| ⋮ button hover tightness | `IconButtonSquare.qml` `padding` / background |
| Quick-edit popup look | `NameDisplay.qml` `editorPopup` (`width: 450`, top Simulated bar, preview, `NameEdit`) |
| Pill size / row gap | `SearchResults.qml` delegate `height: 22`, `Flow.spacing: 2`, label `font 12` |
| Pill tooltip / preview scale | `SearchResults.qml` `ToolTip`; `TilePreview.qml` `sizeMult` |
| Filter order / labels / spacing | `SearchCriteria.qml` (row order, `ColumnLayout.spacing`, `Spacer` heights) |
| Filter panel width (label clipping) | `SearchRoot.qml` `SearchContainer.width` |
| Simulated group spacing / combo width | `NameFullHeader.qml` `RowLayout.spacing`, `SimulatedTilesetCombo` `Layout.preferredWidth`; `FlatToggle` paddings |
| Toggle button look (square/flat) | `general/FlatToggle.qml` (paddings, colors) |
| Full-keyboard footer height / margins | `FullKeyboard.qml` footer `height` (+44), `exampleControls.anchors.topMargin` |
| Trainer-card field height (all fields) | `CardFront.qml` `fieldH` (28); `PlaytimeEdit.qml` forwards it to its sub-edits |
| Trainer-card row spacing | `CardFront.qml` per-field `anchors.topMargin` (spacer→money 18, inter-row 4) |
| Playtime clock field width | `PlaytimeEdit.qml` `digitPad` (2) — applied as left/right padding; width = 2*font.pixelSize + padding |
| Playtime row vertical centering | `PlaytimeEdit.qml` row `height: top.fieldH` + each `PlaytimeDivider` `anchors.verticalCenter` (don't let it size to `childRow.implicitHeight` — that's the Material ~48px, fields then ride high) |
