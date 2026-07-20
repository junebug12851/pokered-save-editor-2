# Full Keyboard — Redesign Plan (ASDF keyboard)

Status: **PLAN / awaiting sign-off** (written 2026-07-11). Nothing implemented yet.

Replaces the current full-screen name editor
(`screens/modal/FullKeyboard.qml` → `PagedPicker` → `SearchRoot`/`SearchContainer`/`SearchCriteria`/
`SearchResults` + `TilesetPicker`) with a **real keyboard outline**: an ASDF/QWERTY deck whose
letter and number keys each hold **one game tile**, with the physical key it's bound to shown as a
**superscript legend** in the corner. Click the key, or just type it. Modifier combinations
(Shift / Ctrl / Alt) switch **pages**; the pages are also clickable buttons above the deck.

---

## 1. The numbers (why the layout is what it is)

`projects/db/assets/data/font.json` holds **255 tiles** (`ind` 1–255; 0 is the string terminator and
is not a tile). Classified by the app's existing colour precedence
(`normal → control → picture → singleChar → variable → multiChar`, from `SearchResults.determineColor`):

| Category | Count | What it is |
|---|---|---|
| Normal | 70 | A–Z, a–z, space, `( ) : ; [ ] - ? ! . / ,`, `Pk` `Mn` `♂` `♀` `×` |
| Single-Char | 49 | digits 0–9, bold `A`–`I`/`V`/`S`/`L`/`M`, `é`, `'`, contractions (`'d 'l 'm 'r 's 't 'v`), quotes, `…`, `·`, `$`, kana |
| Multi-Char | 7 | `<pkmn>` `<poke>` `<pc>` `<tm>` `<trainer>` `<rocket>` `<......>` |
| Variable | 4 | `<player>` `<rival>` `<targ>` `<user>` |
| Picture | 114 | `tile01`–`tile48`, `tile4D`, box-frames `╔ ═ ╗ ║ ╚ ╝`, `tileC0`–`tileDF`, arrows `⮚ ⯈ ⯆` |
| Control | 11 | `<page> <line> <next> <para> <end> <cont> <cont_> <autocont> <done> <prompt> <dex>` |
| **Total** | **255** | |

**Assignable keys per page:** letters (26) + number row (10) = **36**.
**Pages needed:** ⌈255 / 36⌉ = **8**.
**Modifier combinations available:** Shift / Ctrl / Alt = 2³ = **8**. They line up *exactly* — one
page per combination, no leftovers, nothing arbitrary.

The **Space tile** (` `, ind 127) is the 255th and rides on the deck's real **spacebar**, which is
where every human already expects it. So: 254 tiles across 8 × 36 alphanumeric keys (34 slots left
empty), + Space on the spacebar.

---

## 2. Page order & the mapping doctrine

The category strip's order (minus "All") is the usage/priority order project leadership named:
**Normal → Single-Char → Multi-Char → Variable → Picture → Control.** Pages are ordered by that,
and cheap modifier combos get the high-usage pages.

But keys are **not** filled linearly by category. Three rules, in priority order:

1. **Identity beats everything.** If a tile *is* a key (A, a, 7, `!`), it lives on that key. A user
   should never have to learn where "A" is. This is why page 1 is uppercase A–Z + digits and page 2
   is lowercase a–z — exactly like a real keyboard.
2. **Mnemonic beats ergonomics.** `'s` sits on **S**, `'t` on **T**, bold **B** on **B**,
   `<player>` on **P**, `<pc>` on **C**, `<dex>` on **X**. The superscript legend means an
   awkward-but-memorable key beats a comfortable-but-random one.
3. **Ergonomics breaks ties.** Where there's no identity and no mnemonic, the more-used tile gets
   the better key — home row (`F J D K S L A G H`) first, then the upper row, then the bottom row,
   then the number row (a reach). Frequency here is judged by category order first, then by how
   often the tile actually shows up in names/dialogue.

A tile is **promoted to a better key on a later page** rather than being buried on a bad key of an
earlier page (project leadership's explicit instruction) — e.g. the box-frame glyphs and cursor arrows are
the only genuinely useful Pictures, so they take page 5's **home row** rather than being dumped in
tile order.

### The pages

| # | Modifier | Name | Contents |
|---|---|---|---|
| 1 | — | **Letters** | A–Z uppercase (Normal) + digits 0–9 |
| 2 | Shift | **Lowercase** | a–z (Normal) + the 10 Normal symbols |
| 3 | Ctrl | **Symbols** | bold letters + `é ' “ ” ‘ ’ … · $ ﹕ .` + `: ; [ ] / Pk Mn` |
| 4 | Alt | **Codes** | contractions + `<player>/<rival>/<targ>/<user>` + Multi-Char words + kana |
| 5 | Shift+Ctrl | **Tiles I** | box frames + arrows + `tile01`–`tile1B` |
| 6 | Shift+Alt | **Tiles II** | `tile1C`–`tile3F` |
| 7 | Ctrl+Alt | **Tiles III** | `tile40`–`tile48`, `tile4D`, `tileC0`–`tileD9` |
| 8 | Shift+Ctrl+Alt | **Controls** | `tileDA`–`tileDF` + the 11 control codes |

Pages 1–4 (the four cheapest combos) hold **every** Normal, Single-Char, Multi-Char and Variable
tile — i.e. everything you'd ever put in a name. Pictures and Controls (the two you shouldn't) sit
behind two- and three-key combos.

---

## 3. The key maps

Deck rows: **A** = number row `1 2 3 4 5 6 7 8 9 0`, **B** = `Q W E R T Y U I O P`,
**C** = `A S D F G H J K L`, **D** = `Z X C V B N M`.

### Page 1 — Letters (no modifier)

| Key | Tile | | Key | Tile |
|---|---|---|---|---|
| 1–0 | `1 2 3 4 5 6 7 8 9 0` (Single-Char) | | Q–P | `q w e r t y u i o p` |
| A–L | `a s d f g h j k l` | | Z–M | `z x c v b n m` |
| **Space** | ` ` (Normal) | | | |

Pure identity: the base layer of a real keyboard — press a letter key, get the **lowercase** letter.
36/36 used.

> **Corrected 2026-07-11 (project leadership).** The first cut had uppercase here, reasoning that Gen 1 names are
> all-caps. That was wrong: it's the one place the deck would have contradicted every keyboard its user
> has ever touched. All-caps is what **Caps Lock** is for (§5).

### Page 2 — Uppercase (Shift)

Letters: `Q W E R T Y U I O P / A S D F G H J K L / Z X C V B N M` — identity, exactly like a real
keyboard. Number row = the 10 Normal symbols, matching the real Shift-row where the game has the
glyph:

| Key | Tile | Why |
|---|---|---|
| 1 | `!` | Shift+1 = `!` on a real keyboard ✔ |
| 2 | `?` | the other sentence-ender |
| 3 | `.` | |
| 4 | `,` | |
| 5 | `-` | |
| 6 | `♂` (`<m>`) | |
| 7 | `♀` (`<f>`) | |
| 8 | `×` (`<x>`) | Shift+8 = `*` = multiply ✔ |
| 9 | `(` | Shift+9 = `(` ✔ |
| 0 | `)` | Shift+0 = `)` ✔ |

36/36. **Page 2 is 100 % Normal** — pages 1+2 together are the whole in-game-legal character set.

### Page 3 — Symbols (Ctrl)

| Key | Tile | Key | Tile | Key | Tile |
|---|---|---|---|---|---|
| A | bold `A` | I | bold `I` | Q | `“` (`<o">`) |
| B | bold `B` | L | bold `L` | W | `”` (`<c">`) |
| C | bold `C` | M | bold `M` | R | `‘` (`<o'>`) |
| D | bold `D` | S | bold `S` | T | `’` (`<c'>`) |
| E | bold `E` | V | bold `V` | U | `…` (`<...>`) |
| F | bold `F` | J | `'` (apostrophe) | Y | `·` (`<mdot>`) |
| G | bold `G` | K | `é` (`<e>`) | Z | `﹕` (`<:>`) |
| H | bold `H` | N | `Mn` (`<mn>`) | X | `.` (`<.>`) |
| | | P | `Pk` (`<pk>`) | O | `$` (Pokédollar) |
| 1 | `:` | 2 | `;` | 3 | `[` |
| 4 | `]` | 5 | `/` | 6–0 | *(empty)* |

Ctrl+B = bold B is the same thing it means in every app on earth. 31/36 used.

### Page 4 — Codes (Alt)

| Key | Tile | Mnemonic |
|---|---|---|
| D L M R S T V | `'d 'l 'm 'r 's 't 'v` | the contraction sits on its own letter |
| P | `<player>` | **P**layer |
| O | `<rival>` | **O**pponent (and it sits right next to Player) |
| Y | `<targ>` (Player Usage) | **Y**our usage |
| U | `<user>` (Enemy Usage) | **U**ser code — next to Your |
| K | `<pkmn>` | Po**K**émon |
| E | `<poke>` | Pok**é** |
| C | `<pc>` | **C**omputer |
| J | `<tm>` | prime free key (T and M are taken by contractions) |
| H | `<trainer>` | |
| G | `<rocket>` | |
| F | `<......>` | prime key — `……` is common in dialogue |
| 1–5 | `あ い う え お` | kana strip |
| 6–8 | `ァ ゥ ェ` | |
| A B I N Q W X Z, 9, 0 | *(empty)* | |

26/36 used.

### Page 5 — Tiles I (Shift+Ctrl)

The **six box-frame glyphs are drawn as a box on the keys** — `Q`=`╔` `W`=`═` `E`=`╗`, `A`=`║`,
`Z`=`╚` `X`=`╝` — and the three cursor **arrows take the right home keys**: `J`=`⯈` `K`=`⯆` `L`=`⮚`.
Those nine are the only Pictures anyone actually uses, so they get the best keys on the page.

The remaining 27 keys carry `tile01`–`tile1B` in reading order (number row → `R T Y U I O P` →
`S D F G H` → `C V B N M`). 36/36.

### Page 6 — Tiles II (Shift+Alt)

`tile1C`–`tile3F`, straight reading order: number row (`1C`–`25`), `Q`–`P` (`26`–`2F`),
`A`–`L` (`30`–`38`), `Z`–`M` (`39`–`3F`). 36/36.

### Page 7 — Tiles III (Ctrl+Alt)

Number row: `tile40`–`tile48` + `tile4D`. `Q`–`P`: `tileC0`–`tileC9`. `A`–`L`: `tileCA`–`tileD2`.
`Z`–`M`: `tileD3`–`tileD9`. 36/36.

### Page 8 — Controls (Shift+Ctrl+Alt)

Number row `1`–`6`: `tileDA`–`tileDF`. Control codes on mnemonic letters:

| Key | Code | Key | Code |
|---|---|---|---|
| E | `<end>` | C | `<cont>` |
| L | `<line>` | V | `<cont_>` (alt continue, next to Cont) |
| N | `<next>` | B | `<autocont>` |
| P | `<page>` | D | `<done>` |
| A | `<para>` | R | `<prompt>` |
| X | `<dex>` | | |

17/36 used. The most dangerous codes live behind the most deliberate key combination — you cannot
hit `<end>` by accident.

### Completeness

254 tiles on alphanumeric keys + Space on the spacebar = **255 = every tile, each exactly once, no
duplicates.** This is pinned by a unit test (§6).

---

## 4. Visual design

The current screen's problem is that it's a *list of chips next to a filter form*. The new one is a
**device**: a keyboard deck you look at, not a form you read.

- **Deck.** A rounded neutral chassis (light grey, subtle inner border) holding the key rows,
  centred in the page. Real-keyboard silhouette: number row + `⌫`, `QWERTYUIOP`, `ASDFGHJKL` + `⏎`,
  `⇧ ZXCVBNM ⇧`, and a bottom row of `Ctrl Alt ␣ Alt Ctrl`.
- **Keycaps.** Rounded rect, near-white face on the deck, **1 px border in its category colour**, a
  faint wash of that colour in the face (≈8 %), the **animated tile centred**, and the physical key
  as a **superscript legend** in the top-right corner (muted grey). Hover → border goes full
  category colour, cap lifts (a soft shadow), and the detail pane fills. Press → the cap fills the
  category colour at ~20 % and depresses 1 px.
- **Structural keys** (`⇧ Ctrl Alt ␣ ⌫ ⏎`) are neutral grey caps with no tile. The **modifier keys
  light up in the accent colour whenever they're held or latched** — so the deck itself shows you
  which page you're on.
- **Page strip** — 8 buttons above the deck (the `SegStrip` pattern from the Pokemart screen), each
  showing the page name and its modifier badge (`⇧`, `^`, `⌥`). The active one is filled; holding a
  modifier previews/moves the selection live. Clicking a page latches it (so the whole screen is
  usable with the mouse alone, and on a touchscreen).
- **Left rail — the colour reference.** Replaces the old category filter list. Six rows: colour
  swatch · category name · ⓘ. Hovering the ⓘ shows the category's explanation (the same copy the
  old `HelpDot`s carried). It's a legend, not a control — nothing to click, nothing to get wrong.
- **Right pane — the detail split** (kept, with its divider): hovering or clicking a key fills it
  with the colour bar, the tile's name/alias, its raw code, and its description.
- **Footer** unchanged: the Name / Example toggle, the re-roll button, and the live `NameDisplay`.
- **Header** keeps the "Simulated" group (Outdoor toggle + tileset combo) and the name field.
  The **Grid/Tileset toggle is deleted** and the raw tilemap view goes with it.

### Category colours — unchanged (decided 2026-07-11)

The six `Settings::fontColor*` values stay exactly as they are (Normal grey `#616161`, Single-Char
lime `#9E9D24`, Multi-Char amber `#FF6F00`, Variable green `#388E3C`, Picture blue `#303F9F`,
Control purple `#7B1FA2`). A safe→dangerous re-ramp was proposed and **rejected** — the categories
already read fine and the redesign is about the screen's layout/chrome, not the category identity.
The deck applies them as **borders + a faint face wash**, so they stay legible without shouting.

---

## 5. Interaction

- **Click** a key → its code is appended to the name.
- **Type** on the real keyboard → same thing, and the on-screen cap flashes its pressed state.
- **Hold** Shift / Ctrl / Alt → the deck flips to that page live, and **drops straight back when you
  let go**. Physical modifiers are **momentary — nothing latches**, exactly like the shift layer on the
  keyboard under your hands.
- **Click** a modifier cap or a page button → the page **latches**. A mouse can't hold a chord and
  click a key at the same time; and a latched page is the only way in when the OS eats the chord.
- **⌫ Backspace is token-aware** — it deletes a whole `<code>` (e.g. all of `<player>`), never one
  character out of the middle of one. (In *edit mode*, below, Backspace is an ordinary character
  backspace, as it should be.)
- **⏎ Enter** commits and closes; **Esc** closes.
- **Space** inserts the Space tile.

### Caps Lock (added 2026-07-11 — it's a real one)

Caps Lock is **not** "latch the Shift page". It behaves the way it does on every keyboard:

- **Letters only.** The number row keeps typing digits, so `PIKA2` needs no unlocking. *(This is the
  whole reason it can't just be a latched Shift.)*
- **Ignored under Ctrl/Alt.** Ctrl+B is bold B with the caps light on or off.
- **Shift inverts it.** Caps + Shift = lowercase.

The consequence is that the deck can show two pages at once — uppercase letters above a digit row —
which is exactly what a keyboard does. The rules live in C++ (`FontKeyboard::pageForKey`), pinned by
`tst_font_keyboard`, so the QML just asks. Qt exposes no portable way to *read* the caps light, so if
it was already on before the screen opened, the deck corrects itself from the OS's own answer
(`event.text`) on the first letter you press.

### The two modes — keyboard vs. edit (decided 2026-07-11)

Page 3 is the Ctrl page, so `Ctrl+C` is bold C and `Ctrl+V` is bold V — they can't *also* be
copy/paste. Rather than hide that behind a focus rule, the screen has two explicit modes and says
which one you're in:

- **Keyboard mode** (the default): the deck is live and holds the keys. The name field is a
  **read-only** display of what the deck is building — no caret it won't honour. Backspace removes a
  whole **tile**. The field's **pen** button switches to…
- **Edit mode**: the field becomes an ordinary text field — caret, selection, `Ctrl+C/V/Z`, and a
  character-by-character Backspace. **The whole keyboard fades out and goes dead**: it has no say in
  what you're typing and shouldn't pretend to. The pen is replaced by a **check** (apply) and a
  **cross** (discard), so an edit is something you commit or throw away — never something that
  half-happened. Nothing typed here reaches the name until the check is pressed; the cross puts the
  field back. Enter applies, Esc discards. Leaving edit mode hands the keys straight back to the deck.

### Known OS-level risks (honest ones)

- **Shift+Alt** and **Ctrl+Shift** are Windows' *switch keyboard layout* shortcuts when a user has
  more than one input language installed; **Ctrl+Alt** is AltGr on non-US layouts. In those setups
  the held-modifier shortcut for pages 5–7 may be eaten by the OS. Mitigation: **every page is
  always reachable by clicking its page button or the on-screen modifier caps** — the held-key combo
  is a shortcut, never the only way in. This is a design constraint, not a bug to chase.

---

## 6. Implementation

### C++ (new)

`projects/app/src/mvc/fontkeyboardmodel.{h,cpp}` — `FontKeyboardModel : QAbstractListModel`,
exposed as `brg.fontKeyboardModel`.

- A static, hand-written layout table: `page[8][36]` of font `ind` (0 = empty slot), plus the
  spacebar's tile. **This table is the mapping in §3 and is the single source of truth** — the QML
  never guesses.
- `Q_PROPERTY int page` (0–7); setting it resets the model to that page's 36 keys.
- Roles per key: `fontInd`, `keyLabel` ("A", "7"), `alias`, `code`, `tip`, `category`,
  `categoryColor`, `renderMode`, `isEmpty`.
- `Q_INVOKABLE int pageFor(bool shift, bool ctrl, bool alt)`,
  `Q_INVOKABLE int indForKey(int page, const QString& key)` (physical typing → tile),
  `Q_INVOKABLE QString chopLastToken(const QString& str)` (token-aware backspace).

Why C++ and not a QML table: it's testable (below), it keeps the delegate dumb, and the layout is
data, not markup.

### Rendering the animated tiles (the performance trap)

Each `image://font/...` request runs a **full tileset build** (`TilesetEngine::buildTileset`).
Firing that once per key per animation frame (36 keys × 8 frames) would melt the UI — this is the
same class of problem that already froze the old hover tooltip.

So: request the **whole 16×16 tile sheet once per frame** (`image://tileset/...`, exactly what
`TilesetDisplay` already does) and have every key **clip its 8×8 cell out of that one shared
image** — a classic sprite-sheet: `Item { clip: true; Image { source: sheetUrl; x: -col*w; y: -row*w } }`.
One provider call per frame for the entire deck, and QML's pixmap cache hands the same pixmap to all
36 keys. Tile `ind` → `row = ind / 16`, `col = ind % 16`.

Exceptions (they aren't single tiles):
- **Multi-Char / Variable** keys render via the existing `TilePreview` (expanded, **static frame**) —
  so `<poke>` shows "Poké" and `<player>` shows the actual player name in game font. Static, because
  expanding a Variable every frame is exactly what froze the old tooltip.
- **Control** codes have no glyph at all (length 0) → the cap shows its **alias as text** ("New
  Line", "End of text") in the Control colour.

### QML

New, in `fragments/controls/name-full/`:
- `KeyboardDeck.qml` — the chassis, the rows, the structural keys, modifier state, key capture.
- `KeyCap.qml` — one cap (tile / preview / label + superscript legend + hover/press states).
- `TileGlyph.qml` — the sheet-clipping animated tile renderer.
- `PageStrip.qml` — the 8 page buttons.
- `ColorLegend.qml` — the left colour-reference rail.

Rewritten: `FullKeyboard.qml`, `NameFullHeader.qml` (loses the Grid/Tileset toggle),
`DetailView.qml` (restyled).

Deleted (dead once the deck lands): `PagedPicker.qml`, `SearchRoot.qml`, `SearchContainer.qml`,
`SearchCriteria.qml`, `SearchParam.qml`, `SearchResults.qml`, `TilesetPicker.qml` — and their
`app.qrc` entries. `TilePreview.qml` stays (the deck uses it). The C++ `FontSearchModel` /
`brg.fontSearch` go with them **in the same change** (decided 2026-07-11) — nothing lingers.

`Settings` is untouched: the six `fontColor*` values stay as they are (§4).

### Tests

`projects/tests/tst_font_keyboard` (new), pinning the thing that would be catastrophic to get wrong
silently:

1. **Completeness** — every font `ind` 1–255 appears **exactly once** across the 8 pages + the
   spacebar. No tile is unreachable; no tile is duplicated.
2. **No collisions** — no page maps two tiles to the same key; no page exceeds 36 keys.
3. **Validity** — every mapped `ind` resolves in `FontsDB`.
4. **Identity contract** — page 1 key `A` is `A`, page 2 key `A` is `a`, page 1 key `7` is `7`, …
   (a regression guard on the promise that the alphabet is where you expect it).
5. `chopLastToken` — removes a whole `<code>`, a plain char, and is safe on an empty string.

Plus `tst_qml_screens` must stay green (it loads every screen through the real engine).

### Order of work

1. `FontKeyboardModel` + `tst_font_keyboard` (get the map provably right **before** any pixels).
2. New `Settings` palette.
3. `TileGlyph` (sheet clipping) + `KeyCap` — verify one cap renders and animates cheaply.
4. `KeyboardDeck` (rows, structural keys, modifier/page logic, key capture, token backspace).
5. `PageStrip`, `ColorLegend`, `DetailView` restyle.
6. Rewire `FullKeyboard` + `NameFullHeader`; delete the dead QML + qrc entries.
7. Screenshot pass on **every page** (mandatory manual review — overlaps, clipping, spacing), then
   in-app review with a save loaded.

---

## 7. Decisions (project leadership, 2026-07-11)

1. **Lowercase is the base layer; Shift is uppercase.** The first cut had it inverted (Gen 1 names are
   all-caps) — rejected, and rightly: it contradicted every keyboard the user knows. All-caps is what
   **Caps Lock** is for. (§3, §5)
2. **Caps Lock is a real Caps Lock** — letters only, ignored under Ctrl/Alt, inverted by Shift. (§5)
3. **Physical modifiers are momentary; only clicking latches.** (§5)
4. **Two explicit modes** — keyboard mode (deck live, field read-only, tile-wise Backspace) and edit
   mode (field is a normal text field, keyboard fades out + goes dead, check applies / cross discards),
   with the mode named in words on screen. (§5)
5. **Space rides the spacebar** — the only tile not on an alphanumeric key. (§1)
6. **Category colours stay as they are** — no re-ramp. (§4)
7. **Dead search code went in the same change** — `FontSearchModel` / `brg.fontSearch` and the six dead
   QML files were removed with the redesign. (§6)
