# UI Patterns & Conventions

How this app's QML UI is built and styled, distilled from the sessions-13k–13t polish pass on the
Pokémon details editor. **Read this before doing UI work** so screens stay consistent. The
standing rule (see `../context/principles.md` → "The Quality Bar"): **proper layouts, no
fixed/negative-margin hacks.** When a "things overlap / wrong size" bug appears, the cause is almost
always the Qt 6 Material control-height change (`qt-patterns.md` → "Material 3 control heights") and
the fix is a real layout, not a pixel offset.

Deep mechanics live in `qt-patterns.md` and `fix-patterns.md`; this file is the "how we do UI here"
playbook.

---

## Labeled field rows ("option #2")

The standard "label : control" row. The chosen look: the shaded label box grows to the field's
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
  red, which was rejected for underlines.
- The custom combo `popup` must cap its height or it can't scroll — see `qt-patterns.md`
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
`(count/max)` `Text` anchored to its right) at top, and an `ItemBoxView` filling the rest (15px
left/right inset, anchored straight to the pane bottom). No magic-width wrapper boxes.
`ItemBoxView` is a `ListView`; rows are a **left-aligned** `RowLayout` [**grip handle** | CheckBox |
SelectItem | DefTextEdit count | **delete chip**] (anchored left + vcenter) so each row's checkbox
forms a column directly under the header's check-all button. Pinned to the `rowH`/`comboH`/`textH`
knobs so the differently-sized Material controls vcenter and line up. The "+" add row is the
placeholder delegate; bottom breathing room is a `footer: Item { height: 25 }` (not an empty trailing
`Text`).

**No footer bulk-action bar (removed once drag & drop landed, 2026-06-10).** The old 45px footer of
move-to-top/up/down/bottom + transfer + delete `IconButtonSquare`s (each `visible: model.hasChecked`)
is **gone** — reorder and cross-pane moves are now drag gestures (below), and delete moved onto the
row. The model's `checkedMove*`/`checkedTransfer` slots still exist but are now unused by the UI;
`checkedToggleAll` (header check-all) and `checkedDelete` (group delete via the chip) are still used.
The header check-all `IconButtonSquare` `leftMargin` was bumped `24 → 56` to clear the new grip column
(grip 24 + 8 spacing); it's a visual tuning knob — adjust if the grip width/spacing changes.

### Drag & drop on the items LIST (the list analogue of the Pokémon grid drag)

`ItemBoxView` rows support **drag-to-reorder within a list** and **drag-to-transfer between the two
panes** (bag ↔ PC), the direct analogue of `PokemonBoxView`'s grid drag (read that section first — the
mechanics are identical). The **list-specific differences**:

- **Drag is started ONLY from a left grip handle**, never the whole row — a list row holds interactive
  controls (the `SelectItem` combo and the count `DefTextEdit`) that must keep their clicks/typing, so a
  whole-row drag `MouseArea` would steal them. The grip is the first `RowLayout` column: an `Item`
  (24px) holding a muted `grip-lines.svg` `Image` (`opacity 0.4`, → 0.8 on hover) over the drag
  `MouseArea` (`id: dragHandler`, `cursorShape: Qt.OpenHandCursor`, `preventStealing: true`,
  `drag.target: row.isPlaceholder ? null : content`, `drag.threshold: 8`). Everything else
  (manual-`Drag.active` driving via `maActive`, `Drag.drop()` on release, `content` reparent to
  `Overlay.overlay`, `Qt.callLater` deferred mutation, `DropArea`-per-row dispatch
  `dragReorder`/`dragTransfer`) is copied verbatim from `PokemonBoxView`.
- **`Drag.hotSpot.x` is fixed near the grip (`24`)**, not centered — you always grab the grip on the
  far left, so a centered hotspot would jump the wide row ghost out from under the pointer on lift.
- **The drop caret is HORIZONTAL** (a `Canvas` `dropHint` dashed bar straddling the row's **top** edge,
  `height 6`, `anchors.top` + `topMargin: -3`), marking the gap *before* the hovered row — vs. the
  grid's vertical bar on the cell's left edge. Same overlay-only, no-reflow behaviour.
- **A lifted-card background**: while `content.Drag.active`, a white rounded `Rectangle` (`z:-1`,
  accent 1px border) sits behind the row so the floating full-width ghost reads as a card.
- **Per-row delete chip** (`deleteBtn`): placed **to the right of the count field** (a design decision),
  shown when `cellHover.hovered || itemChecked` (off a `content` `HoverHandler`, `cellHover`). `28×28`,
  `times.svg` `icon 19×27`. **No background at rest** — just an **accent-coloured X** so it
  reads on the white row. A `lit` state (`deleteBtn.hovered || itemChecked`) fills the chip
  `primaryColor` (red) with a **white X** — so a **checked** row shows the chip filled *permanently*
  (reads as "armed for deletion"), exactly like the hover look; **press** darkens it
  (`Qt.darker(primaryColor,1.25)`), 90ms `Behavior on color`. (Unlike the Pokémon grid's always-filled
  chip, the items-row chip is transparent at rest; the icon colour flips with the chip so the X is always
  legible.) **Toggle it with `opacity`
  (+`enabled`), NOT `visible`** — a `visible:false` *layout* item collapses to zero width, so with the
  `fillWidth` combo the whole row REFLOWED on every hover (combo grew/shrank as the chip came and went).
  Keeping the chip permanently in the layout and fading `opacity 0↔1` reserves its slot so nothing
  resizes; `enabled` tracks the same `shown` condition so the invisible chip isn't clickable.
  `onClicked: model.deleteItem(index, itemChecked)`.
- **Reserve the scrollbar lane** so the trailing delete chip isn't under the `ScrollBar` (recurring
  gotcha — see "Scrollable forms" below). The `rowEntry` `RowLayout` spans `anchors.left`→`anchors.right`
  with `rightMargin: 16`, and the `SelectItem` combo is the `Layout.fillWidth` element (capped at its
  normal `font*15`, min `font*7`): when the row is tight the combo shrinks so the delete stays inside the
  reserved 16px; on wide panes the combo just caps and nothing visibly changes. (A plain left-packed row
  put the delete under the overlay scrollbar on narrower panes.)

Backing C++ (`ItemStorageModel`, mirrors `PokemonStorageModel`): `dragReorder(from, to, group)` (in-box
splice + `onReset()`), `dragTransfer(from, to, group)` (`relocateOne` to the paired `destBox` then slide
the appended block to the drop slot; capacity guard, **no** "never empties" guard — an item box may be
empty, unlike the party), `deleteItem(index, group)` (group → `checkedDelete`, else `itemRemove`).
`group` pulls the set from `getChecked()`; `toIndex` is the insertion slot (0..count, insert before, ==
append at count). The two item models are paired via `otherModel` in `bridge.cpp` (like the Pokémon
pair). Regression-guarded in `tst_item_storage_model` (`dragReorder_*`, `dragTransfer_movesToOtherBox`,
`deleteItem_singleAndGroup`).

**Auto-stack on cross-pane transfer, NEVER lossy (2026-06-10).** `dragTransfer` does NOT
blindly create a new dst row — for each moved item, if the destination already holds that item id it
**stacks** (folds the moved amount onto the existing row) instead of inserting a duplicate. It stacks
onto the **LAST** matching dst row (the rule — four Antidotes → the bottom one is the stack
target). **The merge happens ONLY when the whole amount fits under the Gen 1 max 99** — we never clamp a
stack and drop the excess (losing legitimate items is bad UX). If the merge would overflow 99, the item
falls through to the **new-row path** (it becomes its own **2nd row**, full amount preserved, no clamp);
and if even that has no room (dst is row-count full), the transfer is **refused** for that item (it
stays in the source), never silently truncated. A *fitting* stack creates no new row, so it's allowed
even when dst is full; the loop uses `continue`, not `break`, so a later item can still fully-merge into
a full box. Stacked items don't participate in the drop-slot slide (only genuinely new rows do). One
`otherModel->onReset()` at the end refreshes both the slide and the stacked-count display (a stack
mutates an `Item` amount but emits no row insert, so the dst model must reset to re-read it). Group move
/ group delete are unaffected. **Important:** this only governs the *editor's own* moves; **pre-existing
duplicate items in a loaded save are left exactly as-is** (the app fully supports a save holding
multiple rows of the same item — it is not the editor's job to "normalize" someone's save). Tests:
`dragTransfer_autoStacksOntoExisting`, `_stacksOntoLastDuplicate`, `_overflowAddsSecondRow`,
`_overflowRefusedWhenDstFull`.

**Duplicate-pick guard + cross-pane owned total in the `SelectItem` dropdown.** `SelectItem` gained two
optional props — `box` (the pane's `ItemStorageBox`) and `currentItemId` (the row's current item):
- **Duplicate guard (same pane only):** when `box` is set, the dropdown **disables/greys** any item the
  box ALREADY holds, *except* this row's own current item, so the user can't accidentally pick a name
  that's already in the **same** pane (keeps stacking tidy; the other pane is irrelevant). Delegate
  `enabled` is `itemSelectInd >= 0 && !(box && itemSelectInd !== currentItemId && box.hasItemInd(itemSelectInd))`;
  greyed text uses `enabled ? textColorDark : textColorMid`.
- **Owned total across both panes:** each entry's text appends the **total amount owned across bag +
  storage** in parens, e.g. `POTION  (x12)` (`box.amountOfInd(ind) + box.destBox.amountOfInd(ind)`,
  shown only when > 0). So even when *this* pane has none, the user sees what they already hold in the
  other pane. The dropdown is alphabetized/categorized, so it's a reliable quick glance no matter
  whether either box is sorted.
`ItemStorageBox::hasItemInd` and the new `amountOfInd(ind)` (sums all matching rows) were made
`Q_INVOKABLE` for this. The checks are plain method calls (not notifying bindings) but the combo popup
is rebuilt on each open, so they re-evaluate every open — fine for the boxes' small sizes. Defaults
(`box: null`) make both features **inert** on any other screen that reuses `SelectItem`. Neither rewrites
pre-existing duplicate save data; the guard only blocks NEW duplicate picks.

### "View All" overview drawer (Bag screen)

The Bag screen's footer is an **`AppFooterBtn3`** (was `AppFooterBtn2`): the footer tiles equal-width
buttons left→right, so the **leftmost** is **View All** (`th.svg`), then Re-Roll, then Sort. View All
slides in a left panel holding a condensed, **alphabetized** table of every item the save holds and
where it is: `Item` | `Bag` | `Storage`, two right-aligned count columns. A count of **0 is hidden**
(`opacity: count > 0 ? 1 : 0`) so each row shows only the side(s) the user actually has. Rows reserve the
**20px scrollbar lane** on the right (see "Scrollable forms").

**DON'T use a Material `Drawer` here — use a hand-rolled sliding `Rectangle`.** The Material `Drawer`'s
content never sat flush: padding/insets + elevation kept leaving a **white strip above the header** and a
**slim white frame** around the panel, and zeroing `padding`/all four insets/`Material.elevation` +
a custom `background` did **not** fix it (the contentItem still didn't fill edge-to-edge). The reliable
pattern is a plain `Rectangle` we fully control:
- **`viewAllPanel`**: a `Rectangle` (`color: "white"`), `width: Math.min(page.width*0.5, 360)`,
  `height: parent.height`, child of the `Page` (so it covers the panes but not the footer). A
  `property bool shown`; `x: shown ? 0 : -width` with a `Behavior on x` (`200ms OutCubic`) slides it in
  from the left. The accent header is the first child of an `anchors.fill` `ColumnLayout`, so it's flush
  at the very top — **no frame, every edge is ours** (even a 1px right-edge divider was rejected;
  the white panel on the dimmed scrim reads fine on its own).
- **`viewAllScrim`**: a black `Rectangle` `anchors.fill: parent`, `opacity: shown ? 0.4 : 0` (Behavior
  fade), `z` below the panel, with a full-size `MouseArea` (`enabled: shown`) that closes the panel on an
  outside click. Dismiss is scrim-click (no Drawer swipe to fight the bag grip handles).
- **Block input pass-through (both the panel AND the scrim).** Plain `Rectangle`/`Text` don't accept
  events, so a click on the header/empty area — or a **wheel the overview list doesn't consume** (a short
  list at its scroll limit) — would reach the pane BEHIND (Qt walks the z-stack top→bottom for unaccepted
  wheel/hover, so it finds the sibling pane behind). Each of the panel and scrim therefore carries **a
  full-size `MouseArea` + a `WheelHandler` (`onWheel: wheel.accepted = true`) + a `HoverHandler { blocking:
  true }`, all `enabled: shown`**. The `HoverHandler.blocking` is the one that stops the bag rows behind
  from lighting up their **hover delete chip** through the panel (hover passes through plain Rectangles).
  On the panel the `MouseArea` is the lowest child so the list above still gets first crack (it scrolls
  when it can; the handlers only swallow what falls through).
- **Refresh on open:** `onShownChanged: if(shown) brg.itemOverviewModel.rebuild()` — an amount edit via
  the count field writes the `Item` directly and may not emit `itemsChanged`, so rebuild to be sure the
  table is current.

Backed by **`ItemOverviewModel`** (`mvc/itemoverviewmodel.*`, `brg.itemOverviewModel`): a read-only
`QAbstractListModel` that aggregates the two item boxes by item index — summing amounts across any
duplicate rows — into `{name, bag, storage}` rows, **drops both-zero rows**, and **sorts by name**
(same `QCollator` as `ItemStorageBox::sort`). It iterates the BOXES (not the items DB) so glitch/unknown
items in a save still appear, resolving each name via `Item::toItem()->getReadable()`. Rebuilds (full
reset) on either box's `itemsChanged`, plus the explicit rebuild-on-open. Registered in
`bootQmlLinkage.cpp`, constructed in `bridge.cpp` with both boxes. Roles `itemName` / `bagCount` /
`storageCount`. Test: `tst_item_storage_model` `itemOverview_aggregatesSortsHidesZeros`.

### "View All" overview drawer (Pokémon screen)

The Pokémon analogue of the Bag View All (read that first — the **slide-in `Rectangle`
panel + scrim + input-blocking handlers + rebuild-on-open** are copied verbatim). The
footer became an **`AppFooterBtn3`** (was `AppFooterBtn2`): **View All** (`th.svg`, leftmost)
then Re-Roll then Boxes Setup. The differences are all in the content, because this is a
**2-D table** not a flat list:

- **Rows = species** (alphabetized by **species name, not nickname**; same `QCollator` as the
  items overview). **Columns = the Party first, then ONLY the non-empty boxes** (by design
  call — empty boxes would be all-blank columns; 12 always-on columns would never fit). Column
  header labels come from the model's `columns` `QStringList` (`"Party"`, `"Box 1"`, …) so the
  header row and every body row stay aligned off one source.
- **Each cell = the count** of that species in that column, **0 hidden** (`opacity: cnt>0?1:0`),
  exactly like the items table.
- **NEW vs items — a per-cell hover tooltip.** Hovering a non-zero count shows (line 1) the
  **differing nicknames** in that cell spelled out, then an **"…and ×N other(s)"** tail for the
  un-nicknamed remainder; (line 2) a **caught/traded split** (`×A caught, ×B traded`, hiding a
  zero side). A mon is "traded" when `PokemonBox::hasTradeStatus(basics)` (OT name/ID differs
  from the player); "nicknamed" = `hasNickname()` (nickname differs from the species default). The
  **whole string is precomputed in C++** (`PokemonOverviewModel::buildCell`) and handed to QML as
  the `tooltips` role, so the QML just shows it (`ToolTip` styled like the slider tooltip — accent
  bg, 80 ms fade). When nothing in the cell is nicknamed, line 1 is omitted (the count + line 2
  already convey it).
- **Layout = a horizontal `Flickable` wrapping a `Column` of [header row | vertical `ListView`].**
  The species column is a frozen-width first column; the inner `ListView` scrolls vertically while
  the outer `Flickable` (`flickableDirection: HorizontalFlick`) scrolls a wide many-box table
  sideways — orthogonal directions so the two don't fight. Panel width adapts to the column count
  (`tableW = nameColW + colCount*countColW + scrollLane`, clamped to `page.width*0.92`); a narrow
  save shows a compact panel, a many-box save scrolls. Reserve the **16px vertical-scrollbar lane**
  (`scrollLane`) on the right of the rows/divider so the last count column isn't under the scrollbar
  (the recurring "Scrollable forms" gotcha).
- Body delegates use **injected model-role properties** (`speciesName`/`counts`/`tooltips`, like
  Bag.qml's `itemName`/`bagCount`), NOT `required` declarations; the row root lifts them onto
  `rowName`/`rowTips` so the nested count `Repeater` (model: `counts`) can index `rowTips[index]`
  for the matching column's tooltip.

Backed by **`PokemonOverviewModel`** (`mvc/pokemonoverviewmodel.*`, `brg.pokemonOverviewModel`): a
read-only `QAbstractListModel` over the **party + 12 boxes + player basics**. Keyed by raw species
id (so two ids sharing a name stay distinct rows, like the items overview keys by index), name via
`PokemonBox::speciesName()`. Roles `speciesName` / `counts` (QVariantList<int>) / `tooltips`
(QVariantList<QString>); the column labels are a separate `columns` `Q_PROPERTY`. Rebuilds (full
reset) on the party's or any box's `pokemonChanged`, plus the explicit rebuild-on-open (species/
nickname/OT edits happen in the detail editor and don't emit a box `pokemonChanged`). Registered in
`bootQmlLinkage.cpp`, constructed in `bridge.cpp`. Test:
`tst_storage_model` `pokemonOverview_columnsCountsTooltips`.

**Polish pass (2026-06-12, requested):**
- **Zebra rows + columns + row hover.** Alternate **columns** are tinted via a **full-height band
  backdrop** (`Row` of `Rectangle`s behind the `ListView`, so the colour runs past the last row);
  alternate **rows** add a faint semi-transparent stripe over it (`rowTintAlpha`/`colTintAlpha`, both
  ≈0.03–0.05 black so they *layer* into a clean grid, not a loud checkerboard — the species column is
  col 0, untinted). **Whole-row hover** highlight is an accent-tinted overlay driven by a row
  `HoverHandler` (stays true over the child count cells, same reason as the storage grid). The header
  got a faint bar (`rgba 0,0,0,0.05`) to read apart from the body.
- **Species-name prettifying + wider column.** Names render through a QML `fixName()` that mirrors the
  **Pokedex screen's** mapping (`Nidoran<f>`→`Nidoran ♀`, `Nidoran<m>`→`Nidoran ♂`, `Mr.Mime`→`Mr. Mime`)
  so the two screens read the same; `nameColW` was widened to **124** so those don't elide. (The model's
  `NameRole` stays the raw readable so sorting matches the Pokedex's `SortName` exactly.)
- **Sort control in the header.** A small hand-rolled icon button next to the "Species" label **cycles the
  SAME orders as the Pokedex** (`PokemonOverviewModel::sortCycle()` mirrors `PokedexModel::dexSortCycle`:
  **Dex / Alphabetical / Internal**). Default **Alphabetical**. Each `Row` carries `dex`/`id` sort keys;
  `applySort()` re-sorts in place on a model reset.
  - **The button shows the CURRENT order's icon, not one static icon + a tooltip** (the tooltip
    looked bad / cluttered). `PokemonOverviewModel::sortIcon` (a `Q_PROPERTY`) returns the qrc path for the
    active order; the QML binds `Image.source` to it. **No tooltip.**
  - Built as an `Item` { hover/press `Rectangle` (radius 2, the same tight square highlight as
    `IconButtonSquare`) + a centered `Image` (`fillMode: PreserveAspectFit`, capped `sourceSize`) + a
    `MouseArea` }. **Not** a `Button`/`IconButtonSquare`: the three sort PNGs are **non-square** (512×~400),
    and `PreserveAspectFit` guarantees they're never squished/stretched (Button.icon can stretch to the
    icon box). They render full-colour (no `icon.color` tint).
  - **Assets:** `sort-alphabetical/internal/pokedex.png` were dropped in the repo-root `assets/staging/`
    staging folder; copied (Windows-side — bash mount serves stale bytes) into
    `projects/app/assets/icons/sort/{alphabetical,internal,pokedex}.png`, added to `app/app.qrc`, referenced
    as `qrc:/assets/icons/sort/*.png`. New assets in qrc → **Rebuild** (RCC re-embed).
- **Condensed columns (~half width).** The count columns were too wide and the table scrolled
  sideways. Per-column widths now: **`nameColW` 110, `partyColW` 46, box columns `boxColW` 30** (via
  `colW(i)` = `i===0 ? partyColW : boxColW`, used by the header, the column bands, AND the row cells so all
  three stay aligned). Box **headers show just the number** ("Box 3" → "3", bold, via `boxNum()`) since
  "Box 12" can't fit 30px; the Party column keeps its word. `tableW` recomputed from the per-column widths;
  the narrower table fits without horizontal scroll in the common case. (Box-number headers are **not**
  bolded.)

**`fixName()` also applies to the storage GRID cell labels (2026-06-12).** The Pokémon selection screen
(`fragments/screens/pokemon/PokemonBoxView.qml`) shows each mon's nickname or species fallback via
`getMonNickname()`; it now renders the species gender markup so `<m>`/`<f>` show as `♂`/`♀` on the grid too.
**Gotcha:** unlike the Pokedex/View All (which only ever see the title-case DB `readable`, so an exact
`"Nidoran<m>"` match works), the grid label is usually the mon's **nickname**, which for an un-nicknamed mon
is the game's **UPPERCASE default** (`"NIDORAN<m>"`) — the exact-match version silently missed it. So
`PokemonBoxView`'s `fixMonName()` does a **generic marker replace** (`<m>`/`<M>`→` ♂`, `<f>`/`<F>`→` ♀`,
`Mr.Mime`→`Mr. Mime`), case-independent on the base name, applied before the 10-char truncation. (The Pokedex
delegate + View All panel keep their exact-match `fixName()` since they only see the title-case readable.)

### Tools menu + "Boxes Formatted" confirm (Pokémon storage footer)

The storage footer's third button is **Tools** (`wrench.svg`) — it opens a `Menu` (`toolsMenu`, declared
inside the `AppFooterBtn3` instance, `x: footerBar.width - implicitWidth`, `y: -implicitHeight` so it pops
**above** the footer, right-aligned) rather than being a one-off action button. (Was a standalone "Boxes
Setup" toggle button.) Today the menu holds one item, **"Boxes Formatted • On/Off"** (label binds to
`storage.boxesFormatted`); add future storage tools here.

Flipping `boxesFormatted` is **destructive in different ways by direction**, so the menu item opens a
**centered modal confirm `Popup`** (`boxesWarn`, `openFor(target)` where `target` is the value it'll flip
TO; alert-red header bar + `info-circle` tinted light via `MultiEffect`; Cancel + a red Format/Unformat
button). Direction-aware copy:
- **→ Format** (currently unformatted): all boxes except the current one are permanently erased + formatted,
  then all 12 open up — "same as the game, just faster", can't be undone.
- **→ Unformat** (currently formatted): only the current box loads; the others aren't touched but are treated
  as nonexistent (a recoverable *soft delete*) — **but** that space is then free for the game/a later format
  to overwrite, permanently erasing every mon outside the current box.

**Proceeding only flips the flag** (`storage.boxesFormatted = target`) — the save engine
(`Storage::load`/`save`) already replicates the game's behaviour from that one bit (bit 7 of `0x284C`): when
unformatted it loads/saves ONLY the current box, leaving the other boxes' bytes untouched; when formatted it
loads/saves all boxes. **No extra bytes are written here** (byte-fidelity preserved — we toggle exactly the
bit the user is toggling).

**The in-app recovery exception (this drives the warning copy — confirmed in `Storage::load`/`save`).**
`load()` always `reset()`s the boxes then expands the 12 box regions (`0x4000`/`0x6000`) **only if
`boxesFormatted`** (an unformatted save loads its other boxes as *empty*); `save()` likewise **only writes
those regions if formatted** (unformatted leaves them untouched). Flipping the flag in-app does **NOT** clear
memory or reload. So:
- **Within a loaded session, unformatting is fully reversible** — the boxes stay in memory, so re-formatting
  restores every mon exactly (and saving unformatted preserves the file's box bytes too).
- **The loss only becomes real once the save is unloaded/reloaded** (the app then won't re-expand those boxes
  while unformatted → can't recover in-app) **or the game overwrites the freed space**. Reformatting *after* a
  reload writes the now-empty boxes over the preserved bytes → permanent.
The two warning messages spell this out (unformat = soft delete, recoverable by reformat until reload; format
= boxes are saved with whatever's loaded, empty ones overwrite what was there).
- **Tooltip ownership rule (refined).** The caught/traded line is shown **only when something is traded**
  — an all-caught cell omits it (it adds nothing). So a cell with **no differing nicknames AND nothing
  traded yields an empty tooltip**, and the view shows **no tooltip at all** on it (the QML already gates
  `hoverEnabled`/`visible` on `tip !== ""`). Built in `PokemonOverviewModel::buildCell`.

## Pokémon storage screen layout (the standard for this screen)

`screens/non-modal/Pokemon.qml` mirrors Bag: two `PokemonPane` in a **`RowLayout`**
(`spacing: 0`), each `Layout.fillWidth` + `Layout.fillHeight` for a 50/50 split (was
`width: Math.trunc(parent.width * 0.50)` + chained anchors). Each `PokemonPane`
(`fragments/screens/pokemon/`) is a `Rectangle` with an anchored 45px **header bar**
(check-all `IconButtonSquare` parked at the bar's left edge `leftMargin: 24`; a
`SelectPokemonBox` switcher `anchors.centerIn`; an **`Active` `FlatToggle`** anchored to
the switcher's right) at top, and a `PokemonBoxView` filling the rest (15px
left/right inset, 15px bottom margin). Removed the old `width: 265` magic-width wrapper
`Row` and every repeated `leftPadding/rightPadding/leftInset/rightInset: 0` override on
the `IconButtonSquare`s (use them bare per "⋮ icon menu buttons" above). Note: keep the
toggle a **sibling** of the combo (not nested) — when the old dot button was briefly nested
*inside* the combo, `model` rebound to the combo's model; it must reference `top.model.curBox`.

**Box-header "Active" toggle + box-dropdown decoration (2026-06-10, built
clean in repo `build/`, kit-dir rebuild pending for in-app review).** Replaced two
not-self-explanatory affordances with clearer ones:
- **The fill circle is gone** from the box dropdown. `PokemonBoxSelectModel::getDecoratedName`
  no longer prepends `●`/`○`/blank — the `(N/Max)` count it already appends conveys fullness
  more precisely. The unused `boxEmptySym`/`boxNotEmptySym`/`boxFullSym` members were removed.
- **The current-box marker moved + flipped.** It used to be a trailing `◁`; it's now a *leading*
  `▷` (where the circle used to sit, pointing at the label). `curBoxSym` is the only decoration
  symbol left. Non-current rows pad with 3 spaces so the list stays left-aligned.
- **The "set current box" target/dot button became an `Active` `FlatToggle`.** It shows On
  (filled) when this pane's box *is* the game's current/active storage box, Off (outlined)
  otherwise — self-describing state, not a cryptic target icon. Rule: a save always
  has exactly **one** active box, so the toggle is **`enabled: !active`** — you click an Off
  toggle to activate that box (the other pane's toggle then reads Off); you can't turn the active
  box off, only switch which box is active. **Hidden on the Party pane** (`visible: top.model.curBox >= 0`)
  — the party has no active-box concept. `onClicked` sets
  `brg.file.data.dataExpanded.storage.curBox = top.model.curBox`.
- **`FlatToggle` is now color-parameterized** (`toggleColor`/`activeTextColor`/`inactiveTextColor`/
  `hoverColor`), defaults reproducing the original look so the keyboard toggles are unchanged. The
  header instance sits on the **accent bar**, so it inverts them (light border/text, light fill +
  accent text when On) to read against the colored header. If a flat toggle ever needs to live on a
  colored surface again, override these four — don't fork the component.

**No footer bulk-action bar (removed once drag & drop landed).** The old 45px footer of
move-to-top/up/down/bottom + transfer + release `IconButtonSquare`s (each `visible:
model.hasChecked`) is **gone** — reorder and cross-pane moves are now drag gestures, and
delete moved onto the cell (below). The model's `checkedMove*`/`checkedTransfer` slots
still exist but are now unused by the UI; `checkedToggleAll` (header check-all) and
`checkedDelete` (group delete) are still used.

**Checkbox selection — scoped persistence (by design exact rule).** Selection should survive
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

**HP bar** (`hpBar`, a narrow `Rectangle` track + fill, between the icon and the name label):
`height 5`, `radius height/2`, 14px left/right insets, dark `Qt.rgba(0,0,0,0.22)` track; the fill
width is `parent.width * clamp(itemHp/itemHpMax)` and its colour matches the **editor's HP slider
exactly** (`GlancePane.qml` `hpEdit.getColor`): **`>0.5` green `#4CAF50`, `>0.2` amber `#FFA000`,
else red `#D32F2F`** (fraction = current `hp` / computed-max `hpStat`). Backed by two new model
roles `itemHp`/`itemHpMax` (`PokemonStorageModel` `HpRole`/`HpMaxRole` → `mon->hp` / `mon->hpStat()`).
The icon's `anchors.bottom` is `hpBar.top` so icon → bar → name stack as one unit; `visible` only for
real mons with `itemHpMax > 0`. (No live `dataChanged` for HP needed — the grid resets when the
detail editor closes, which is the only place HP changes.)

**Status condition badge** (`statusIcon`, an `Image` upper-left of the cell, `26×26`, `topMargin/
leftMargin 3`): shows the mon's status condition. New model role `itemStatus` (`StatusRole` →
`mon->status`, the raw gen-1 byte). `cell.getStatusIcon(s)` bit-decodes it (sleep `0x07`, poison
`0x08`, burn `0x10`, freeze `0x20`, paralyze `0x40` — matching `StatusSelectModel`'s values
1-7/8/16/32/64) to `qrc:/assets/icons/status/{sleep,poison,burn,freeze,paralyze}.png`. **Shares the
top-left corner with the hover/checked checkbox** — `visible` is gated on `itemStatus > 0 &&
!cellHover.hovered && itemChecked !== true` (z 90, below the checkbox's z 100) so the two never
overlap: status at rest, checkbox when interacting. Assets are the ChatGPT-made framed badges
(1254×1254 **RGBA / transparent**; `sourceSize` caps the decode to the display size).
Source PNGs live in `projects/app/assets/icons/status/` and are listed in `app/app.qrc` (new files →
**must** be in the qrc or they 404 at runtime). **Import workflow:** revised art is dropped in
the **repo-root `assets/staging/`** staging folder (sleep-icon/poisoned/burned/frozen/paralyzed); copy +
rename into the app tree, then rebuild (touch `app.qrc` so AUTORCC re-embeds changed bytes). That
staging folder's **contents are gitignored** (`assets/staging/.gitignore` = `*` + `!.gitignore`, so the
folder stays tracked but the dropped files don't). **Do the copy on the Windows side, not bash** — the
Cowork bash mount serves stale cached bytes for these (a bash `cp` grabbed the pre-revision art twice;
see [[feedback_bash_mount_stale]]).
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
between the two panes**. The chosen interaction (decided up front): **insert at the
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
- The **drop indicator is an insertion caret** (a design decision): a `Canvas` (`dropHint`) drawing a
  **dashed vertical bar straddling the cell's LEFT edge** (`width: 6`, `anchors.left` + `leftMargin:
  -3` to center on the cell boundary, `lineWidth 3`, `setLineDash([5,4])`, round caps),
  `visible: cell.containsDrag`. It marks the **gap before** the hovered cell — i.e. *between* entries —
  and is a pure overlay, so **icons never shuffle or resize** while dragging (explicitly
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

## Pokemart (shop) screen layout (the standard for this screen)

`screens/non-modal/Pokemart.qml` is the buy/sell shop over `brg.marketModel` (four modes:
buy/sell × money/coins). It's a **two-pane** `RowLayout` (2026-06-15): the **shopping list**
on the left and a store-style **receipt** on the right, with a 1px `dividerColor` divider
between. (The first pass that day was an internal-only modernization; the two-pane receipt
was the follow-up redesign Twilight asked for.)

- **Layout = a full-width header + a switching body.** The two segmented strips live in a **full-width
  accent header** (not the left pane). Below, the body switches on `isExchangeMode`: **Buy/Sell** → the
  two-pane shop (list + receipt); **Exchange** → a centered **converter card**. The shop list/receipt and
  the converter each bind their `model:` to `null` in the other mode so hidden delegates aren't built.
- **Exchange = a money<->coins CONVERTER card** (not a list). Centered white card: **MONEY (left) ⇄ COINS
  (right)**, each a big live balance (`exchangeMoneyAfter`/`exchangeCoinsAfter`) with a **±Δ above**
  (`deltaStr` vs `…Start`) and a Material accent **`Button` under it**: **"+ Coins"** (buys 1 coin, by
  money) on the coins side, **"+ Money"** (sells 1 coin, for money) on the money side — each with the
  **per-coin rate beneath** (`₽<buyRate>/coin`, `₽<sellRate>/coin`). The buttons drive one **net axis**
  via `brg.marketModel.exchangeAdjust(±1)` (+Coins = +1, +Money = -1, so they cancel) / `exchangeNet()`;
  `autoRepeat` for hold-to-bulk; `enabled` gated on the money/coin caps + affordability. Rates come from
  `exchangeBuyRate`/`exchangeSellRate`. **Rate truth:** a coin costs `getBuyPrice()` (₽20) — the model's
  `ItemMarketEntryMoney` treats `onCart` as *coins* (cost = rate·coins); don't invert it.
- **`StepPill` inline component** is the shared `-/qty/+` pill (borderless `DefTextEdit`, a `live` guard so
  the initial fill doesn't write the model mid-incubation, and a `Connections` re-seat on external value
  change). Used by both shop rows and converter lanes — don't re-inline a stepper.
- **Mode controls = two segmented strips** (not footer buttons, not a title). An
  inline `component SegStrip` (a connected single-select control styled for the accent bar: selected =
  light fill + accent text, light outline, dividers; `options` array, bound `currentIndex`,
  `stripEnabled`, `picked(index)` signal). Two of them: **action** `[Buy|Sell]` → `isBuyMode`, **disabled
  while Exchange is selected**; **venue** `[Pokemart|Game Corner|Exchange]` → `isMoneyCurrency` /
  `isExchangeMode` (index 2). The footer is a single **Checkout** `AppFooterBtn1`. **Selected-segment
  corners:** the strip has `radius` but `clip` is *rectangular* (won't round child corners), so the
  selected fill uses **per-corner radius** (`topLeftRadius`/… , Qt 6.7+) on the first/last segment to
  follow the rounded ends. *(Home tile label is "Market"; the venue tab stays "Pokemart".)*
- **One unified Buy+Sell cart per currency.** Within a venue the list holds BOTH the sell rows (your items)
  and the buy rows (the store) at once, and the cart spans both — the Buy/Sell strip only filters the left
  VIEW, it does **not** split or clear the cart (`isBuyMode` no longer triggers a rebuild). Each row's
  direction is intrinsic (`cartSignVal` member: sell +1, buy -1); `totalWorth()` is the **signed** net and
  the receipt itemizes with `+`/`-` per row (`dataCartSign`) and one net total. The left `ListView` binds
  to **`brg.marketViewModel`** (`ItemMarketViewModel`, a proxy that slices the unified list to the active
  view via `viewTag`); the receipt still binds to `brg.marketCartModel` (cart across both). **Aggregates
  sweep the current model's own list (`ItemMarketEntry::activeList`), never the global static registry** —
  see `qt-patterns.md` → "Global static object registries → cross-app use-after-free".
- **Exchange is its own mode.** `isExchangeMode` builds a dedicated list (a header + the two fixed-direction
  `ItemMarketEntryMoney` rows via `forceDir`); the money exchange was **removed** from the buy/sell lists.
  In Exchange the receipt shows a **dual-currency** Money / Coins `start → after (Δ)` summary (not the item
  receipt), driven by `exchangeMoney/CoinsStart/After` (which mirror the money rows' checkout deltas
  exactly). Buy/sell affordability for a swap is gated by `ItemMarketEntryMoney::canCheckout()` on the
  currency actually spent (the base single-currency `moneyLeftover()` can't).
- **Left pane = the shopping list** (segmented-strip header + a `ListView` of proper item rows).
  Each item row is a left-aligned `RowLayout` (14px left, 16px scrollbar lane): **name** (`fillWidth`,
  elide) · **owned `xN`** (sell only) · **unit price** (right-aligned, min-width 54) · a **stepper
  "pill"** as the row action (`-`/qty/`+` in a rounded `Rectangle` that turns white on hover; the
  `DefTextEdit` qty is borderless via `background: Item {}`). A whole-row `HoverHandler` drives an
  **accent-tinted hover highlight** (0.12 alpha) with faint **zebra** (`index % 2`) otherwise. `"msg"`
  rows are **uppercase section headers** on a tint bar. Sizing knobs on the page root: `rowH`/`headH`/
  `qtyW`. The list carries **no running total** (the receipt totals it). *(This replaced the earlier
  centered-stepper delegate — do not bring back the screen-centered stepper with labels fanned around
  it; rows are plain left-aligned list rows now.)*
- **Right pane = the receipt** (`brg.marketCartModel`). An accent "Cart" header (+ a live `xN`
  count), then a `ColumnLayout`: "Money on hand" → divider → an **itemized `ListView`** (one line
  per carted item: name + signed line total on top via `moneyStr(dataCartWorth,false,true,type)`,
  `xQty @ unit` beneath; 16px scrollbar lane; "Your cart is empty." placeholder) → divider → bold
  **Total** (`moneyStr(totalCartWorth,true,true)`) → **Balance after** (`moneyLeftover`) → a wrapped
  **warning** line (`errorColor`) shown only when `warningText() !== ""`. The receipt is read-only —
  all quantity control stays on the left. There is **no** floating summary box or bottom status bar
  anymore (both folded into the receipt).
- **`ItemMarketCartModel`** (`mvc/itemmarketcartmodel.*`, `brg.marketCartModel`) is a
  `QSortFilterProxyModel` over `ItemMarketModel` filtering `cartCount > 0` and dropping `"msg"` rows;
  it inherits the source role names (so the receipt delegate uses the same `data*` roles) and
  re-filters live off the source's `dataChanged`. Model-wide totals stay on `ItemMarketModel`.

- **Item-row layout** is described in the "Left pane" bullet above (left-aligned columns + a trailing
  stepper-pill action, hover/zebra backgrounds, uppercase section headers). *History:* the very first
  pass used a fragile `Rectangle { width: 1; height: 50 }` with everything chained off siblings; the
  second pass made it a plain `Item` with the `-/amount/+` stepper `anchors.centerIn: parent` (screen-
  centered, labels fanned around it). Both are **superseded** — don't reintroduce the width:1 trick or
  the screen-centered stepper.
- **One text per adaptive label, not N visibility-toggled copies.** The receipt's warning line comes from
  `warningText()` (buy/sell) or `exchangeWarningText()` (exchange); don't fan these back into one `Text`
  per case with `visible:` flags (the old anti-pattern). *(The old mode-title `headerText()` is gone — the
  segmented strips replaced the title.)*
- **Currency helpers live on the page root** (`maxMoney`/`curSym`/`signing`/`moneyStr`/`moneyColor`) —
  presentation/formatting only. `moneyColor` returns **`brg.settings.errorColor`** for the error red.
  Use that token for plain red — **not** `brg.settings.primaryColor` (that's *pink*, `#d81b60`) and not a
  literal `"red"`. `errorColor` is a fixed, theme-independent red defined next to the primary palette in
  `Settings` (`setColorScheme()` leaves it untouched), so it survives the upcoming theming work. See
  `../../projects/app/src/bridge/settings.h`.
- **No floating summary box / status bar.** The earlier internal-modernization pass had a slide-down
  `summaryScreen` `Rectangle` and a bottom accent status bar; both were **removed** when the receipt
  pane landed — the receipt now carries money-before / total / balance / warnings.
- **Breathing room** at the left list's bottom is a `ListView { footer: Item { height: 16 } }` (was a
  per-delegate `isLast` empty `Text`) — same idea as the Bag/Pokémon lists.
- **Footer disabled-button highlight:** the Checkout button (`btn2.enabled: canAnyCheckout`) used to
  stick in its hover highlight when it disabled under the cursor — fixed at the root in
  `FooterButton.qml` (`hoverEnabled: enabled`); see `qt-patterns.md` → "Disabled control keeps its
  hover highlight".

## Pokémon details — Moves tab (the standard for this tab)

`fragments/screens/pokemon-details/MovesTab.qml` + `PokemonMoveSel.qml`. Same grouped-panel language as
the General/DV-EV tabs: a `ScrollView` → one white bordered panel → a `Repeater` of the **four move
slots** as zebra rows, with the **16px scrollbar lane** reserved on the `ColumnLayout` (see "Scrollable
forms"). `MovesTab` owns the **layout + drag**; `PokemonMoveSel` owns the **controls** (so the row drag
mechanics don't fight the combo/PP field).

- **Row look (type accent, not a pill) + fixed columns.** The old whole-row type-colored pill is gone.
  Each filled row keeps its type identity with a **colored left accent strip** (`getColor()`, the
  Bulbapedia CC-BY-NC-SA palette — keep the attribution) + a **fixed-width faint type chip**
  (`Qt.lighter(getColor(),1.35)`, width 58). The **move name combo fills** the remainder; everything to
  its right is **fixed width** so the name + type columns line up across rows (don't make the type chip
  implicit-width — that's what made the rows jitter). Empty slots show just the combo (so a move can be
  picked) and a faint neutral strip so the column edge stays aligned.
- **PP / PP-Ups dual view — TWO INDEPENDENT boxes (anti-desync), max-only arrow.** A tab-level `showPpUps`
  (`SegSel` `[PP | PP Ups]` in the top bar) flips what each row edits: current/max **PP**, or current/max
  **PP-Ups** (max 3). **Do NOT use one shared text box that writes `pp` or `ppUp` depending on the view** —
  that desyncs: the `maximumLength` flip on toggle truncates the text and fires a cross-field write, and
  the "max" buttons feed the same broken box. Instead use **two boxes** (`movePP<i>` / `movePPUp<i>`), each
  bound to + writing ONLY its own field, swapped by a `StackLayout` (`currentIndex: showPpUps ? 1 : 0`).
  Each box has its own `→|` "set to max" button (`restorePP()` / `maxPpUp()`); the **min/reset arrow was
  dropped** for space. Size both boxes for 2 digits so the width doesn't jump on toggle. (Regression-tested
  in `tst_gui_moves`: toggling never mutates data, the boxes are independent.) Note `onMoveIdChanged`
  legitimately **clamps `pp` to `getMaxPP`**, so a typed PP above the cap snaps down — expected.
- **Per-move actions = a HANDLE-TRIGGERED, FULL-COVER reveal (no reflow).** Each row's `[dice | magic |
  trash]` group (randomize `monMove.randomize()` / make-valid `correctMove()` / delete
  `boxData.deleteMoveAt(index)`, which **compacts**) lives behind a small **chevron handle** (`angle-left`)
  in a reserved right lane (`mainRow.rightMargin: 26`, so the value box is visible + editable at rest).
  Hovering the handle OR the slid-in panel (`handleHover`/`panelHover`) reveals it. The panel's backing is
  the **row colour** (`rowColor`, from `altRow`) — it just hides the value box behind it, with a hairline
  left edge — width 132 so it **completely covers** the value box + max + `/max`; it slides via `x`
  translate (`width` → `0`) and the handle fades as it covers. The tools themselves are the **exact same
  bordered icon group as the tab's top bar** (`RowBtn { padding:7; icon 18 }`, dark icons, even widths — NOT
  `accentColor` / white / `fillWidth`, which read out of place and sized unevenly). It's an overlay (not in
  the layout) over a `clip:true` root, so revealing it **never reflows** the row. Don't trigger the reveal
  on whole-row hover and don't only partly cover the value box — both were rejected. Icons: make-valid is
  **`file-circle-check`**, NOT `check-double` (that's the select-all icon); delete + the top-bar
  clear-all-but-first share **`trash-alt`**. **Consistent order both places: validate · random ·
  destructive** — per-row `[validate | random | delete]`, top bar `[validate | random | clear-all]`.
- **Bulk top bar.** Carries the view toggle plus `[file-circle-check | dice | trash-alt]` (validate ·
  random · clear, matching the per-row order): make all valid (`correctMoves()`), randomize all
  (`randomizeMoves()`), clear-all-but-first (`clearMovesButFirst()`). The
  broom's `enabled` tests `boxData.movesAt(1).moveID > 0` (because `movesCount()` is a plain C++ method, not
  QML-callable). Reuse the DV/EV `SegSel`/`SegBtn` components; the per-row buttons are the same flat-segment
  idea (`RowBtn`).
- **`PokemonMoveSel` needs an explicit `boxData` property.** A separate component can't see the parent
  file's properties by bare name, and `PokemonMove::parentMon` is a plain C++ member (not a `Q_PROPERTY`)
  so it isn't reachable from QML — `MovesTab` passes `boxData` down for the All-Moves ops + species hook.
- **Drag-to-reorder = the Bag list drag, on a fixed 4-row Repeater.** Read "Drag & drop on the items
  LIST" first — the mechanics (grip-only drag via a `MouseArea` whose `drag.target` is a `content` Item,
  manual `Drag.active`/`Drag.drop()`, reparent `content` to `Overlay.overlay`, lifted-card bg, dashed
  caret, `Qt.callLater` deferred mutation) are copied. Tab-specific differences:
  - **Grip only on FILLED moves** (`row.filled`); empties park at the bottom (game move-list compaction)
    and aren't draggable. The grip column (24px) is always reserved so every combo lines up.
  - **The drop slot uses the pointer's vertical half** (`DropArea.onPositionChanged` → `dropAfter =
    drag.y > height/2`): top half inserts before the row, bottom half after — so the **lower half of the
    last move appends to the end** (no trailing "+" placeholder exists here). The caret `y` flips to the
    row's bottom edge when `dropAfter`.
  - Backed by **`PokemonBox::reorderMove(from, to)`** (Q_INVOKABLE): it anchor-splices the **filled**
    slots' `(id/pp/ppUp)` **values** among the **fixed** `moves[4]` objects (the same anchor convention as
    `PokemonStorageModel::dragReorder`) and writes them back via `PokemonMove::changeMove`. The slot
    QObjects keep their identity, so QML's `movesAt()` pointers stay valid; PP rides with its move; only
    the reordered move bytes change (byte-fidelity — `tst_gui_fidelity` guards it). `toIndex == movesCount`
    appends; an empty/out-of-range `from` is a no-op.
- **Null-safety is mandatory here** (the Repeater delegate reads through `boxData`/`monMove`, both
  transiently null during build & in the `tst_qml_screens` inject-then-complete path). Guard EVERY
  `monMove.` deref — including the `Menu` items, which evaluate eagerly — and don't read tab constants
  through the root `top` id inside a delegate (that warns `Unable to assign [undefined]` mid-build): inline
  the literal tint / use `row.height`, and coerce `top.boxData ? … : null`. See `qt-patterns.md` →
  "Repeater delegates read through transient-null bindings".

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

**Same gotcha on a `ListView`/`GridView` with a vertical `ScrollBar` and a trailing control in each
row** (e.g. the items-row delete chip — 2026-06-10). The scrollbar overlays the right ~16px of the view,
so a right-most row control lands under it. Fix the row the same way: span the row layout to the view
width minus 16 (`anchors.right` + `rightMargin: 16`) and make a middle element (`SelectItem` combo there)
`Layout.fillWidth` so the trailing control is pulled inside the reserved lane when space is tight. **This
keeps coming back — whenever you add a right-edge control to a scrollable list/grid, reserve the 16px.**

The Pokémon details **General tab** (`OverviewTab.qml`) was the latest to get this (2026-06-16): it had
shipped with `width: scroller.availableWidth` (no `- 16`), so its Nickname / OT Name / OT ID ⋮ menus sat
under the scrollbar. Fixed to `- 16`, and the Exp slider's one-off `Layout.rightMargin: 25` was dropped in
favour of the same lane (a `fillWidth` slider in a lane-reserved column — no per-control magic margin).

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

## The full keyboard's DECK (2026-07-11 — the current design)

⚠️ **This supersedes the "pill grid" / filter-sidebar / tilemap section below.** Those files
(`SearchResults`, `SearchContainer`, `SearchCriteria`, `SearchParam`, `SearchRoot`, `PagedPicker`,
`TilesetPicker`) and the C++ `FontSearchModel` are **deleted**. The section is kept for the parts still
in force (`TilePreview`, `NameDisplay`, the commit-on-finish rule) and as history.

`screens/modal/FullKeyboard.qml` is now a **keyboard**, not a search form. Full design + the whole
tile→key map: [`../plans/full-keyboard-redesign.md`](../plans/full-keyboard-redesign.md).

- **The map lives in C++, not QML** (`mvc/fontkeyboard.{h,cpp}` → `brg.keyboard`). 8 pages × 36 keys,
  hand-authored. QML never guesses which tile is on a key; it asks `keyData(page, "A")` and gets
  `{ ind, code, title, tip, category, render, empty }` — an *empty* key comes back as a well-formed
  empty map, never a null the delegate has to guard.
- **A page's index IS its modifier mask** (shift 1, ctrl 2, alt 4), so Alt is page **4**. `pageFor()` is
  just the mask. The human reading order (by category, cheapest chord first) is the separate
  `pageOrder` — the page strip renders in *that*. Confusing the two silently swaps two whole pages
  (it did; `tst_font_keyboard` caught it).
- **Every tile appears exactly once across the pages + the spacebar**, and that is a *test*
  (`tst_font_keyboard`), not a comment. A duplicated tile is bad; a tile stranded with no key is
  unreachable-forever and would never show up as a crash or a warning.
- **Layout:** `PageStrip` spans the FULL body width (eight named chips do not fit a middle column at
  the app's default 750×480 window), then a row of `ColorLegend` (132) · `KeyboardDeck` (fills) ·
  `DetailView` (212). The deck's key unit `u` scales to fit: width/13.5, height/6.0 — those constants
  are the deck's real extent in key units (10 caps + a 2u Backspace + 11 gaps + chassis padding). Get
  them wrong and the chassis silently overflows and paints **on top of** the legend and the detail pane.
- **Animated tiles: ONE shared sheet, not one request per key** (`TileGlyph.qml`). Every
  `image://font` request rebuilds the whole tileset — 36 keys × 8 frames would melt the UI (it's the
  same cost that froze the old hover tooltip). So the deck asks for the whole 16×16 sheet once per
  frame (`image://tileset/...`, as `TilesetDisplay` does) and each key **clips its 8×8 cell** out of it
  (`row = ind/16, col = ind%16`). Same URL for all keys ⇒ QML's pixmap cache serves one pixmap. **One
  timer on the deck** drives `curFrame` for every cap — never a timer per delegate.
- **A cap draws a tile OR a label, never an expanded preview.** Multi-char/variable codes
  (`<player>`, `<trainer>`) expand to 7+ characters — rendered on a ~30px cap they draw wider than the
  key and **smear across their neighbours** (tried; unusable). Those caps and the control codes show
  the bare code (`<player>` → `player`); the *detail pane* renders the real expanded glyphs at a
  readable size.
- **The key legend** (which physical key types this tile) is a small superscript in the cap's corner.
  It **dims when the name field takes focus** — that's the visible signal that the deck has handed the
  keyboard over, so the mode is never hidden.
- **Physical modifiers are MOMENTARY; clicking latches.** Hold Ctrl → the page flips; let go → it drops
  straight back, like the shift layer on a real keyboard. Clicking an on-screen modifier cap (or a page
  button) *latches*, because a mouse can't hold a chord and click a key at once — and because a latched
  page is the only way in when the OS eats the chord (Windows takes Shift+Alt / Ctrl+Shift on
  multi-language setups; Ctrl+Alt is AltGr). Held and latched are OR'd, so nothing can disagree.
- **Caps Lock LOCKS THE SHIFT PAGE** (`FontKeyboard::effectivePage`, pinned by `tst_font_keyboard`):
  Shift inverts it, Ctrl/Alt ignore it. So **every state the deck can be in is exactly one of the 8
  pages** — which is what lets the page strip always be right.
  *Rejected first:* the real-keyboard rule (caps affects the 26 letters only, number row keeps typing
  digits). It's what a physical keyboard does, but it produces a layer that **isn't one of the pages**,
  the strip can't name it, and it reads as a bug ("why are there two different page 2s?"). **A model the
  UI can't display is a bad model, however correct it is.** The cost of the fix is that the punctuation
  row rides along with caps (tap caps off to type a digit — rare, and the deck shows the change).
- **The base layer is LOWERCASE.** `a–z` unshifted, `A–Z` on Shift — like every keyboard. Uppercase-
  unshifted was tried (Gen 1 names are all-caps) and **rejected**: that's an argument for a good Caps
  Lock, not for inverting the alphabet.
- **Two explicit modes, named on screen** (`NameFullEdit` owns the state). *Keyboard mode*: the deck is
  live and the field is **read-only** (a caret in a field you can't type into is a lie); Backspace eats a
  whole tile. *Edit mode*: the pen makes the field an ordinary text field (caret, selection, Ctrl+C/V/Z,
  character-wise Backspace) and **the whole lower half — strip, legend, deck, detail — fades to 0.25 and
  goes `enabled: false` as one thing**. The pen becomes check (apply) / cross (discard); nothing typed
  there reaches `str` until the check. This replaced a *hidden* mode (click the box and the deck quietly
  stopped listening).
- **Backspace is token-aware** (`FontKeyboard::chopLastToken`): it deletes a whole `<code>`, never one
  character out of the middle of one — that would leave a string the codec can't round-trip.
- **A key that won't fit shakes the name field** (`NameFullEdit.reject()`) rather than doing nothing.
  It animates a `Translate`, **not** the field's `x` — `x` belongs to the RowLayout, and fighting a
  layout over a position gives you a control that never sits still.
- **Re-announce the hovered key when the page changes** (`KeyCap.onInfoChanged`): switching page under a
  stationary mouse swaps the tile with no enter/exit, so the detail pane would keep describing the tile
  that used to be there.
- **Never name a component root `id: top` when it has a Repeater** — see `qt-patterns.md`. It cost real
  time here: the bindings read `undefined`, the item gets a NaN width, and it renders as *nothing* with
  no warning at all.

### The deck's LOOK — the rules the first cut broke (2026-07-11)

Twilight's verdict on the first pass was *"looks really bad but i cant place my finger on why"*. Every
cause was nameable, and each one is a rule worth keeping:

- **Figure/ground.** Light caps on a light chassis on a light pane = mush. The chassis is a **dark
  slate** (`Qt.darker(accentColor, 1.55)`) and the caps are light. Structural keys (Caps/Shift/Ctrl/Alt/
  Enter/⌫) are **dark with light text**, so the eye separates *keys that type* from *keys that do*; a
  held/latched modifier goes **bright**.
- **If a colour only appears on hover, it isn't doing anything.** The category tint was a 6% wash behind
  a 35% border — invisible until you hovered, which defeats the entire point of a colour legend. Caps
  now carry a real wash of their category colour (`Qt.lighter(cat, 1.82)`) with a solid border.
- **The key legend has to be readable**, or "just type it" is undiscoverable. It scales with the key
  (`height * 0.30`), full opacity, in a dark shade of the cap's own colour. (The glyph is nudged
  −2/+2 off centre so the legend isn't sitting on it.)
- **Draw the whole silhouette, even the keys you don't use.** 36 caps floating in a block reads as
  *worse* than a real layout, even though it's roomier. The deck draws the full ANSI outline — `` ` ``
  `-` `=` Tab `[` `]` `\` `;` `'` `,` `.` `/` Win Menu — as **dead keys** (`StructKey.dead`): muted,
  inert, no hover, no cursor, no clicks. Pure silhouette, and it's what makes the thing read as a
  keyboard.
- **A blank key looks broken.** The spacebar was rendering the Space *tile*, which is (correctly)
  nothing — so it read as disabled. It says **"Space"** across it.
- **Chrome takes room from the thing the page is for.** The old header (132px) + footer (119px) of
  washed-out `lighter(accent, 1.5)` blue ate half a 480px window and squeezed the keyboard into the
  rest. Both are now a clean light surface with a hairline divider, at 88px / ~104px.

## Full keyboard + quick-edit patterns (s13v–s13z8) — HISTORICAL

⚠️ Superseded by the deck (above) as of 2026-07-11. The pill grid, the category filter sidebar and the
tilemap view described here **no longer exist**. Still in force: `TilePreview`, `NameDisplay`, and the
commit-on-finish rule.

The font editors — the modal full keyboard (`name-full/*`, `screens/modal/FullKeyboard.qml`) and the
quick-edit popup (`general/NameDisplay.qml`) — were rebuilt across sessions 13v–13z8. Conventions:

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
  `qt-patterns.md`). **Control pills get no tooltip at all.**
- **Single-select radio filters** (`SearchCriteria.qml`, `SearchParam.qml`, `SearchContainer.qml`):
  `RadioButton`s in a shared `ButtonGroup` (auto-exclusive only works among one parent's siblings, and
  these live in separate rows). Order: **All, Normal Only, Single-Char, Multi-Char, Variable** then a
  gap then **Picture, Control**. Backend = `FontSearch::keepAnyOf(...)` (union; one selected = that
  category); **All** calls `startOver()`. No Clear button (one is always active). "Normal **Only**"
  subtly signals that leaving it leaves the always-safe set. *(History: started tristate → AND s13v →
  OR/union s13y → radios s13z3; a design decision each time.)*
- **Compact filter rows**: Material `CheckBox`/`RadioButton` floor at ~40px — trim `topPadding`/
  `bottomPadding` + `Layout.minimumHeight: 0` to halve the spacing.
- **Help on a ⓘ dot, not the whole row** (`SearchCriteria.qml` `HelpDot`): a `Label "ⓘ"` with its own
  `HoverHandler` driving a `MainToolTip` (default placement). Hovering the checkbox/row should NOT
  trigger it. **Reserve scrollbar width** in the panel (`SearchContainer` `criteria.width = topz.width
  - 16`) so the right-aligned dots aren't under the scrollbar.
- **`FlatToggle`** (`general/FlatToggle.qml`): the flat, **square** (radius 0), **no-shadow**
  (`Material.elevation: 0`, custom background) toggle button — filled accent when `active`, outlined
  otherwise. Used for Outdoor, Grid/Tileset (keyboard), and Name/Example (both editors). Rejected
  Material's elevated/rounded buttons here.
- **Paged view toggle, not swipe** (`PagedPicker`): `SwipeView` dots/gestures (clip over
  content). Keep `SwipeView { interactive: false }` for the slide, drive `currentIndex` from a header
  `FlatToggle` (`showTileset`) whose label reflects the **current** view ("Grid" / "Tileset").
- **`SimulatedTilesetCombo`** (`general/`): the app-wide tileset picker (drives
  `brg.settings.previewTileset`). In `general` so the keyboard header AND the popup reuse it (was
  `name-full/NameFullTileset` — moved to avoid a `general → name-full` import cycle).
- **Editors are menu-free** (s13z7): the rule — *UI isn't first, UX is*; the old ⋮ overflow menu
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
  `qt-patterns.md`.

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

## Credits / About screen

`screens/modal/About.qml` renders `brg.creditsModel` (now **section-grouped**: one row per category,
roles `section` + `entries`). Layout conventions:

- **One translucent card per category.** A `ListView` (spacing 18) whose delegate is a rounded
  `Rectangle` (`radius 12`, `color Qt.rgba(1,1,1,0.88)`, 1px `Qt.rgba(0,0,0,0.06)` border, subtle
  `MultiEffect` drop shadow). Card width is capped + centered via `top.contentWidth`
  (`Math.min(width-48, 580)`), shared by the `header` (title + intro) and `footer` (version + copyright)
  so everything lines up in one column. Inner content sits in a `Column` inset by `card.pad` (22).
- **Section icon.** A bundled Font Awesome svg mapped by heading in `About.sectionIcon()` (presentation
  only — kept out of `credits.json`). Tint a black FA svg to the palette: an `Image { visible:false }`
  as the source of a sibling `MultiEffect { colorization:1.0; colorizationColor: … }` (here
  `primaryColor`). `map` is a placeholder for **Wallpapers** — swap if a better fit is added.
- **Clickable links.** Render the URL as `textFormat: Text.StyledText`, `text:'<a href="…">…</a>'`,
  `linkColor: primaryColor`, `onLinkActivated: (l)=>Qt.openUrlExternally(l)`. URLs in the data omit the
  scheme — `About.linkHref()` prepends `https://`. A `MouseArea { acceptedButtons: Qt.NoButton;
  cursorShape: PointingHandCursor }` gives a pointer without swallowing the click.
- **Font-size variation is intentional** (by design): heading 22 bold, entry name 16 bold, note 14,
  mandated/url 13 italic, license 12 italic; greys via `textColorDark`/`textColorMid`.
- **Version/copyright** comes from `Qt.application.name`/`.version` (set in `boot.cpp`) — don't hardcode.
- The data + back end are data-driven: add a credit by editing **only** `credits.json` (ordered
  `sections` array). `CreditDBEntry::process()` loops it; the flat store is regrouped by `CreditsModel`.

## Connected action-button "combo" groups + grouped form rows (Pokémon details General tab)

The Pokémon details **General** tab (`OverviewTab.qml`, 2026-06-17 redesign) is the reference for a
form that reads as a cohesive grouped list rather than "a pile of fields next to a pile of buttons":

- **Per-field actions = a connected segmented control**, not loose icons. `component SegBtn: Button`
  (flat, `IconOnly`, `padding: 7`, `icon.color: textColorDark`, `Layout.fillHeight`+`minimumHeight: 0`)
  is one segment with a square hover/press fill and a 1px **left divider** (drawn except when
  `first: true`). Wrap N of them in a bordered, rounded, `clip: true` `Rectangle` holding a
  spacing-0 `RowLayout` → the group reads as one Material control. Used for `[randomize | clear]`
  (Nickname/OT/Type) and `[randomize]` (Nature). The group is right-aligned in one action column
  (an `Item { Layout.fillWidth: true }` before it) so the buttons line up and stay clear of the
  scrollbar. (This replaced both the ⋮ overflow menus and the loose `IconButtonSquare`s.)
  - **trash-alt / X / other tall-narrow glyphs only "render as a sliver" when the button is clipped
    off the pane edge** — that was a layout (right-aligned-into-the-scrollbar) bug, not an icon bug.
    Inside a properly-sized group they render fine.
- **Rows grouped in one panel with zebra + muted labels.** A single `Rectangle` panel (white,
  `clip: true`, border optional — dropped here so rows fill it) holds a spacing-0 `ColumnLayout` of
  row `Rectangle`s. Alternate rows tint `Qt.rgba(0,0,0,0.04)` (zebra); labels are muted right-aligned
  `Text` (`component RowLabel`, fixed 90px) instead of the chunky shaded "option #2" boxes. This is the
  Bag/Market "cohesive list" look applied to a heterogeneous form.
- **Slider rows with end markers above the bar.** Exp shows **Lv. N / Lv. N+1** (a `RowLayout` of two
  `Text`s with an `Item{fillWidth}` between, above the slider in a `ColumnLayout`); Catch Difficulty shows
  **Easy / Hard** the same way. Both show their value on hover+press (the standard slider tooltip). Catch
  Difficulty is **inverted** (`value = 255 - byte`, tooltip shows the byte) so Easy (high catch rate) is on
  the left.
- **GlancePane stat column needs an `implicitWidth`.** The stats `Rectangle` (`StatsGroup`/`StatsGroupInvalid`)
  must expose `implicitWidth: grid.implicitWidth` (and be `color: "transparent"`) so `GlancePane` can size the
  stats column and anchor the sprite to its right — without it the rect is 0-wide and the sprite overlaps the
  stats (only visible once the pane is narrowed).

### Segmented **text** selector (`SegSel`) + the "active = data binding" rule (DV/EV tab)

The **DV/EV** tab (`StatsTab.qml`, 2026-06-17) extends the same language to *mode* and *state*
selectors with `component SegSel: Button` — the text-label sibling of `SegBtn`. It's a flat,
**non-checkable** `Button` (`TextOnly`, `Layout.fillHeight`) whose `background` fills with
`accentColor` when a `property bool active` is true (text → `textColorLight`), else a hover/press
wash; a 1px left divider shows except when `first` or `active`. Wrap N in the same bordered/rounded/
`clip:true` `Rectangle` + spacing-0 `RowLayout` as `SegBtn`. Used for `[DV | EV]` and
`[Shiny | Non-Shiny]`; the `[Max | Re-Roll | Reset]` action combo reuses `SegBtn`
(angle-double-up / dice / angle-double-down, retargeted by `statKind`).

- **`active` must be a binding to the underlying value, never a `checkable` toggle.** When the
  selection has to mirror *live data that can change without clicking the control* — Shiny/Non-Shiny
  flips when you **drag a DV slider**, not only when you click a segment — `checkable` + `ButtonGroup`
  is wrong: a click toggles `checked` and severs the binding. Instead bind `active: boxData.isShiny`
  (and `active: !boxData.isShiny` on the sibling), and let `onClicked` perform the *action*
  (`makeShiny`/`unmakeShiny`), which mutates the data, which re-drives `active`. One source of truth,
  zero drift. Same shape for the `[DV|EV]` switch (`active: statKind === "DV"`,
  `onClicked: statKind = "DV"`).
- This removed the tab's last `CheckBox` **and** its ⋮ menus — the details editor is now ⋮-menu-free.

### `RandomButton`'s optional clear segment + the shared `SegToggle` (trainer card, 2026-07-10)

The trainer card adopted the same combo language, and two reusable pieces came out of it:

- **`RandomButton` gained a `showClear` flag** (default **false**). Off, it's the lone rounded dice
  exactly as before (Starter, Player ID). On, it appends a second **trash** `SegBtn` so the group reads
  as one **[dice | trash]** pill and emits `clear()` (with `clearTip`). Money/Coins clear to 0; the
  playtime clock uses `randomizePlaytime()` / `clearPlaytime()`. The trailing segment is toggled with
  **`visible`** (a `!visible` `Layout` item collapses to zero width, so the group narrows back to just
  the dice) — one source of truth, every screen's randomize/clear button identical. This replaced the
  playtime's hover-reveal ⋮ overflow menu.
- **`SegToggle` (`fragments/general/SegToggle.qml`)** is the **independent on/off** sibling of
  `SegSel`: same flat accent-fill-when-`active` look and per-corner rounding, but each segment is its
  own boolean — `active` binds to a bit and `onClicked` flips it (SegSel's segments are mutually
  exclusive; SegToggle's aren't). The trainer card's `PlaytimeToggles` wraps two of them in the
  standard bordered/rounded group as **[Enabled | Paused]** (bound to `playtime.clockMaxed` and
  `area.general.countPlaytime`, both inverted), sitting just above the clock. Same "active = data
  binding, never `checkable`" rule as SegSel.
- **Layout note:** always-visible action groups need horizontal room the hover-menu didn't. Adding the
  second (trash) segment overflowed the card border until the right-hand column was shifted left
  (Player ID `rightMargin` 65 → 92) and the top spacing tightened to fit the extra toggle row — verified
  against the headless trainer-card screenshot, not by eye.
- **`PlaytimeGroup` — a light titled "grouping box" (2026-07-10 redesign).** The playtime cluster reads
  better wrapped in one labelled group than as loose rows. `PlaytimeGroup.qml` is a rounded, 1px
  `rgba(0,0,0,0.22)`-bordered `Rectangle` (title "Playtime" top-left, muted bold) holding: row 1 =
  `PlaytimeEdit` (now **clock digits only** — the button was pulled out) + the shared `RandomButton`
  re-anchored beside it (instance `anchors.left: clock.right` overrides the component's default
  "off parent's right"); row 2 = `PlaytimeToggles` **below** the fields. It sizes to content
  (`implicitWidth`/`Height` from the clock+button row) and is right-aligned in `CardFront` so its border
  lines up with the Money/Coins action buttons. Reusable takeaway: to put a `RandomButton` *inside* a
  layout instead of floating off a field's right edge, just re-anchor it at the instance.
- **The trainer card was widened 500×250 → 600×310 for this.** The old width let the clock butt up
  against / overlap the trainer artwork; more width gives the right column + Playtime group clear
  separation from the art, more height fits the two-row group. **Lesson (the important one): capture and
  actually scrutinise the screenshot** — the overlap existed in the first action-groups pass and was only
  caught when Twilight pointed it out. Manual screenshot review (overlaps, alignment, clipping, polish)
  is a standing default for any card/screen layout change, not a glance.

### Segmented active segment: round the fill's outer corners (the "Market" flat-edge)

When a segment in a rounded, `clip:true` group is filled (an **active** `SegSel`, or any hover/press
wash on an **end** segment), its corner shows up **square** where the group is rounded — because
`clip: true` on a `Rectangle` clips to the *bounding box*, not the rounded radius. The Pokémart
segmented strips have this same bug. Fix: don't rely on clip — give the segment's **fill** per-corner
radii (Qt 6.7+): `topLeftRadius`/`bottomLeftRadius` when the segment is `first`, and
`topRightRadius`/`bottomRightRadius` when it's `last` (a `last` flag added alongside `first`), each
equal to the group's `radius`. Middle segments keep all-0. Now the fill follows the group's rounded
corners. Used by both `SegSel` and `SegBtn` in `StatsTab.qml`; the same fix would clean up the
Pokémart strips.

### Custom "to-line" icons (`arrow-left-to-line` / `arrow-right-to-line`)

For min/max-style actions, `|←` (to minimum) and `→|` (to maximum) read far better than vertical
chevrons. They weren't in the FA subset, so they're **hand-authored** SVGs in
`assets/icons/fontawesome/` (a `<rect>` bar + an arrow `<path>`, viewBox `0 0 512 512`, **plain black
fill** so a Button's `icon.color` tints them like the real FA icons). Any new icon must also be added
to `app.qrc` (the file list is explicit, no wildcard) — then RCC rebakes it on the next build.

## The map screen (2026-07-12 — step 1 of the map emulator)

The map screen draws **nothing of its own invention**. Every number comes from `brg.map` (MapModel) in
**buffer pixels** — one screen pixel per Game Boy pixel, 32 to a block, origin at the top-left of the
border ring — and QML's only job is to multiply by an integer `zoom`. If a rectangle looks wrong,
the bug is in C++ (`MapEngine`), not here. Keep it that way.

Conventions established here:

- **Integer zoom only.** This is 8x8 pixel art; a fractional scale smears it. `Image { smooth: false }`,
  and the provider scales with `Qt::FastTransformation` when asked at all.
- **Default zoom is fit-to-window** (`userZoom == 0`), with a **Fit** button to return to it. Opening a
  screen onto the corner of a map is not showing someone their map. Big maps (Route 17 is 78 blocks tall)
  land on 1x and scroll — correct, they simply are bigger than the window.
- **The grid line is TINTED, not grey.** The map is four shades of grey, so a grey line disappears into it
  — over the black trees or over the white paths, depending which grey you pick. A low-alpha colour has
  nothing to hide against and reads everywhere. (This is why the block grid was invisible on the first
  pass; the *other* reason is the Repeater landmine below.)
- **Three boxes, three theme colours:** the visible screen = `errorColor`, the draw/scratch area =
  `accentColor`, the map bounds = `primaryColor`. A footer legend names them, because three nested
  coloured rectangles are meaningless without one.
- ⚠️ **The root id is `mapScreen`, NOT `top`** — this file has Repeaters, and a Repeater delegate
  cannot see the file's root id. `top.zoom` inside a delegate is `undefined` -> `x` goes NaN ->
  every line silently collapses onto x = 0, with **no warning and a green `tst_qml_screens`**. Inside a
  delegate, reach values through a plain sibling id (`canvas.gridStep`). See
  [`qt-patterns.md`](qt-patterns.md) -> "`id: top` + a Repeater delegate".
