# Fix Patterns — Quick Reference

When you see a compiler or runtime error, find it here.

## C++ Compiler Errors

| Error | Fix |
|-------|-----|
| `XxxDB::store` private | `XxxDB::inst()->getStore()` |
| `XxxDB::store.size()` | `XxxDB::inst()->getStoreSize()` |
| `XxxDB::ind.value(k, nullptr)` | `XxxDB::inst()->getIndAt(k)` |
| `XxxDB::ind.value(k)` | `XxxDB::inst()->getIndAt(k)` |
| `getIndAt(key, nullptr)` (2 args) | `getIndAt(key)` — 1 arg only |
| `Random::rangeExclusive(...)` static | `Random::inst()->rangeExclusive(...)` |
| `Random::rangeInclusive(...)` static | `Random::inst()->rangeInclusive(...)` |
| `Random::flipCoin()` static | `Random::inst()->flipCoin()` |
| `MapsDB::search()` static | `MapsDB::inst()->search()` |
| `FontsDB::convertToCode(...)` static | `FontsDB::inst()->convertToCode(...)` |
| `FontsDB::convertFromCode(...)` static | `FontsDB::inst()->convertFromCode(...)` |
| `Utility::decodeAfterUrl(...)` static | `Utility::inst()->decodeAfterUrl(...)` |
| `FontsDB::expandStr(...)` static | `FontsDB::inst()->expandStr(...)` |
| `FontSearch::results` private | `search->getFontCount()` / `search->fontAt(i)` |
| `entry->fieldName` protected | `entry->getFieldName()` |
| `entry->ind` protected | `entry->getInd()` |
| `entry->name` protected | `entry->getName()` |
| `entry->readable` protected | `entry->getReadable()` |
| `entry->glitch` protected | `entry->getGlitch()` |
| `entry->once` protected | `entry->getOnce()` |
| `entry->toSprite` protected | `entry->getToSprite()` |
| `entry->toTrainer` protected | `entry->getToTrainer()` |
| `entry->toItem` protected | `entry->getToItem()` |
| `entry->toPokemon` protected | `entry->getToPokemon()` |
| `MapDBEntry::ind` protected | `entry->getInd()` |
| `MapDBEntry::width/height` protected | `entry->getWidth()` / `entry->getHeight()` |
| `MapDBEntry::warpIn` protected | `entry->getWarpIn()` |
| `MapDBEntry::warpOut` protected | `entry->getWarpOut()` |
| `MapDBEntry::sprites` protected | `entry->getSprites()` |
| `MapDBEntry::signs` protected | `entry->getSigns()` |
| `MapDBEntry::toTileset` protected | `entry->getToTileset()` |
| `MapDBEntry::toMusic` protected | `entry->getToMusic()` |
| `MapDBEntry::toSpriteSet` protected | `entry->getToSpriteSet()` |
| `MapDBEntry::glitch/special` protected | `entry->getGlitch()` / `entry->getSpecial()` |
| `pickRandom()->ind` | `pickRandom()->getInd()` |
| `*map->width` (was optional) | `map->getWidth()` (plain int, -1 = unset) |
| `if(map->width)` (optional check) | `if(map->getWidth() >= 0)` |
| `SpriteType::TRAINER` | `MapDBEntrySprite::SpriteType::TRAINER` |
| `ConnectDir::EAST` etc. | `MapDBEntryConnect::ConnectDir::EAST` |
| `new FontsDB` (private ctor) | `FontsDB::inst()` |
| `new Utility` (private ctor) | `Utility::inst()` |
| `new NamesDB` | `NamesPlayer::inst()` |
| `new NamesPokemonDB` | `NamesPokemon::inst()` |
| `new ExamplesPlayer/Pokemon/Rival` | `ExamplesPlayer::inst()` etc. |
| `NamesDB::randomName()` | `Names::inst()->player()->randomExample()` |
| `NamesPokemonDB::randomName()` | `Names::inst()->pokemon()->randomExample()` |
| `Qt::CTRL + Qt::Key_X` | `Qt::CTRL \| Qt::Key_X` (operator+ deleted in Qt 6) |
| `QString::SkipEmptyParts` | `Qt::SkipEmptyParts` |
| `QString::SplitBehavior::KeepEmptyParts` | `Qt::KeepEmptyParts` |
| Missing `DB_AUTOPORT` → linker undefined symbol | Add `DB_AUTOPORT` macro + `db_autoport.h` include |
| `undefined symbol: XxxDB::getSomething()` | Declared but not implemented — add body to .cpp |
| `area.h:NN: expected unqualified-id` at a lone `{` | Recovery **collapsed a block + ate the class-declaration line**: `area.h` had its 10-line `Q_DECLARE_OPAQUE_POINTER` block + `class MapDBEntry;` + `class SAVEFILE_AUTOPORT Area : public QObject` crushed down to a single stray `Q_DECLARE_OPAQUE_POINTER(AreaAudio*)` directly followed by `{`. Restored from HEAD. Hunt for this flavor: scan headers for a `Q_OBJECT` not preceded by any `class`/`struct` decl within ~5 lines (a small Python pass), and compare `Q_DECLARE_OPAQUE_POINTER` counts vs HEAD (but note that phrase also appears in explanatory *comments* → comment-only count diffs are false alarms). |
| `is_complete<T>::value` static_assert / "Pointer Meta Types must point to fully-defined types or be declared with Q_DECLARE_OPAQUE_POINTER" (savefile moc) | Recovery dropped the **full `#include`s** of QML-traversed property-pointer types from the owning headers, leaving only forward declarations. In the unity moc build that makes the pointee incomplete (and poisons the type's own metatype downstream — the `is_complete<Self>` errors are secondary). Fix = restore the full `#include` at the owning header (the documented convention for traversed types; opaque/non-traversed types stay in `savefile_autoport.h`). Recovery dropped these (all confirmed against HEAD): `player.h`←playerbasics/playerpokedex/playerpokemon/itemstoragebox; `savefileexpanded.h`←player/area/world/storage; `world.h`←worldother; `filemanagement.h`←savefile; `playerpokemon.h`←pokemonparty; `storage.h`←itemstoragebox/playerbasics/pokemonstoragebox; `savefile.h`←savefileexpanded; `mainwindow.cpp`←pse-db/db.h. Find dropped includes fast: `diff <(grep '#include' f) <(git show HEAD:f | grep '#include')`. |
| `ld.lld: undefined symbol: DB::qmlProtect/qmlHook`, `FontSearch::clear/keepAnyOf` (referenced by moc) | Recovery dropped the **method bodies** (header still declares them, so moc references them → link error). Recover the real bodies, don't guess. Source priority: working-tree `.cpp` for structure, then `git show <ref>:<path>` (HEAD first, then `af883fd`) — but note **both can be truncated**, so verify. `DB::qmlProtect` calls `qmlProtect(engine)` on all 26 sub-DBs (accessor order in `db.cpp`); HEAD's copy was cut off at `SpritesDB`, completed from the full list. `DB::qmlHook` mirrors `Utility::qmlHook` → `context->setContextProperty("pseDB", const_cast<DB*>(this));`. Before adding N sub-calls, confirm each callee is actually **defined** (`grep '::qmlProtect' each .cpp`) or you trade 1 link error for N. |
| `QFileDialog` not found | Add `Qt6::Widgets` to CMakeLists target_link_libraries |
| `Q_DECLARE_OPAQUE_POINTER` parse error | Add `#include <QMetaType>` before the macro |
| `convertToCode` returns `QVector<int>`, need `QVector<var8>` | Cast element-by-element |
| `GameCornerDB::buyPrice` private | `GameCornerDB::inst()->getBuyPrice()` |
| DB constructor static-init deadlock | Remove `load()` from DB constructor — `DB::loadAll()` calls it |
| `'./abstracthiddenitemdb.h' file not found` (AUTOMOC, from `util/hiddencoinsdb.h`) | Recovery artifact: a stray truncated `hiddencoinsdb.{h,cpp}` was placed in `pse-db/util/` (where `abstracthiddenitemdb.h` doesn't exist) and added to CMakeLists. Canonical files are `pse-db/hiddencoinsdb.{h,cpp}` (next to the abstract base). Removed the `src/pse-db/util/hiddencoinsdb.cpp` line from `db/CMakeLists.txt`; the orphaned `util/` copies are inert. Lesson: after a bulk recovery, check for duplicate source basenames (`find … | sed 's#.*/##' | sort | uniq -d`). |
| `'buyPrice' is a private member` + `reference to non-static member function must be called` + `expression is not assignable` (`gamecornerdbentry.cpp`) | Recovery glitch in `gamecornerdb.h`: the private **data members** `QVector<GameCornerDBEntry*> store;` and `int buyPrice = 0;` were mangled into bogus method decls `int buyPrice() const; int sellPrice() const;` (and the `store` member + `GameCornerDBEntry` friend were dropped). Truth is in the `.cpp`: `getBuyPrice()` returns `buyPrice`, `getSellPrice()` returns `buyPrice/2` (no `sellPrice` member), `store` used throughout. Restored the two members + `friend struct GameCornerDBEntry;` (for the `inst()->buyPrice = price` write in the entry ctor). |
| `'toGameCorner' is a protected member of 'ItemDBEntry'` | Same deep-link friend pattern as MapDBEntry: the writer must be befriended. Added `friend struct GameCornerDBEntry;` to `ItemDBEntry` (for `toItem->toGameCorner = this` in `GameCornerDBEntry::deepLink`). |
| `use of undeclared identifier 'store'` / `'ind'` (or "did you mean 'strrev'?") in a `*db.cpp` | Recovery dropped the private container members from the DB header. Restore them in the `private:` section, e.g. `fontsdb.h` lost `QVector<FontDBEntry*> store;` and `QHash<QString, FontDBEntry*> ind;`. Confirm exact members via the `.cpp`'s `getStore()`/`getInd()`/`getStoreSize()` bodies (and `git show HEAD:<header>`). NOTE: this is the **DB-class** `store`/`ind` (containers). An *entry* class's `getInd()` returns its `int ind` field — different thing, not this bug. |
| `no viable conversion from QScopedPointer<const FontSearch,...> to QScopedPointer<FontSearch,...>` (`fontsdb.cpp` `search()`) | Recovery typo: body built `QScopedPointer<const FontSearch, …>` but the declared return type is non-const. Drop the stray `const`. |
| QML load fail: `Type X unavailable ... Cannot assign to non-existent property "onSomeSignal"` | Recovery left a **mismatched QML pair** — a parent wires `onSomeSignal:` on a child that no longer declares that `signal` (an earlier-iteration leftover). Verify where the behavior actually lives now (here: the example toggle/`>>` moved to `FullKeyboard`'s footer; `NameFullEdit` no longer emits `toggleExample`/`reUpdateExample`), then delete the dead wiring on the child. Don't re-add the signal unless that's truly where it belongs (UX = Twilight's call). |
| **Runtime: fonts/pills blank out, go red/empty, names stop saving after interaction** (no crash) | The s13f `DB::inst()->qmlProtect(engine)` call in `MainWindow::injectIntoQML()` was dropped by recovery (the *definition* in db.cpp was also dropped — see other row). Without it, QML garbage-collects the shared parentless DB entries (FontDBEntry etc.) mid-session → dangling/null → blank fonts, red/empty picker pills, names stop saving until reboot. Restore `DB::inst()->qmlProtect(engine);` at the end of `injectIntoQML()` (needs `#include <pse-db/db.h>`). Non-deterministic GC = symptom looks random (e.g., only the pills you navigated to). |
| **Runtime hang** (no crash/error) when rendering a name/example containing a variable tile (`<player>`/`<rival>`/`<pkmn>`…), e.g. "next random example" | `FontsDB::splice()` lost its `out.remove(position);` (recovery reverted the s13y "replace-not-insert" fix; af883fd had `out.remove(ind)`). Without it, `expandStr` re-expands the same tile forever. Re-add `out.remove(position);` before the insert loop. Verified against af883fd. |
| `definition of implicitly declared default constructor` (`Names::Names()` etc.) | Recovery dropped the constructor **declaration** from the header. The `.cpp` defines `X::X()` but the `.h` no longer declares it. Re-add it — for singleton DB classes it goes in a `private:` section (`Names();`), per "all DB classes have private constructors". |
| `use of undeclared identifier 'results'` / `'qmlRegister'` (`mapsearch.cpp`) + later `out-of-line definition does not match` | Recovery **truncated the header's tail**: `mapsearch.h` lost `getMaps()/mapAt()/qmlProtect()/qmlRegister()` and the `QVector<MapDBEntry*> results;` member, and `getMapCount` was left non-`const` vs the `.cpp`'s `const`. Reconstruct from the `.cpp` signatures + the parallel `fontsearch.h` (QML-interface block → `public slots: qmlProtect` → `private slots: qmlRegister` → `private: QVector<…> results;`). NOTE: clang stops at ~20 errors, so the pasted log undercounts — scan the whole `.h`↔`.cpp` for missing decls + const-mismatches in one pass (a small Python regex compare works). |

## Missing Includes

When you get "incomplete type" or "member access into incomplete type":

| Type | Include to add |
|------|---------------|
| `MapDBEntry` | `#include <pse-db/entries/mapdbentry.h>` |
| `MapDBEntryConnect` | `#include <pse-db/entries/mapdbentryconnect.h>` |
| `MapDBEntryWarpOut` | `#include <pse-db/entries/mapdbentrywarpout.h>` |
| `MapDBEntryWarpIn` | `#include <pse-db/entries/mapdbentrywarpin.h>` |
| `MapDBEntrySign` | `#include <pse-db/entries/mapdbentrysign.h>` |
| `MapDBEntrySprite` | `#include <pse-db/entries/mapdbentrysprite.h>` |
| `MapDBEntrySpriteItem` | `#include <pse-db/entries/mapdbentryspriteitem.h>` |
| `MapDBEntrySpriteTrainer` | `#include <pse-db/entries/mapdbentryspritetrainer.h>` |
| `MapDBEntrySpritePokemon` | `#include <pse-db/entries/mapdbentryspritepokemon.h>` |
| `MapDBEntryWildMon` | `#include <pse-db/entries/mapdbentrywildmon.h>` |
| `ItemDBEntry` | `#include <pse-db/entries/itemdbentry.h>` |
| `GameCornerDBEntry` | `#include <pse-db/entries/gamecornerdbentry.h>` |
| `FontDBEntry` | `#include <pse-db/entries/fontdbentry.h>` |
| `EventDBEntry` | `#include <pse-db/entries/eventdbentry.h>` |
| `MissableDBEntry` | `#include <pse-db/entries/missabledbentry.h>` |
| `NamesPlayer` | `#include <pse-db/entries/namesplayer.h>` |
| `NamesPokemon` | `#include <pse-db/entries/namespokemon.h>` |
| `ExamplesPlayer/Pokemon/Rival` | `#include <pse-db/entries/examplesplayer.h>` etc. |

## Runtime / QML Errors

| Error | Fix |
|-------|-----|
| QML: `Xxx is not a type` / `Type Xxx unavailable` for a **new** `.qml` file | This project resolves QML types via `app/app.qrc`, not directory scanning — a new `.qml` file must be added as a `<file>` entry in `app.qrc` (and rebuilt) before it resolves as a type, even though debug builds load existing QML from disk. (s13v: `TilePreview.qml`.) |
| QML: keyboard filter behavior (current, s13z3+) | Filters are **single-select radio buttons** in a shared `ButtonGroup` (incl. an **All**). Backend: `FontSearch::keepAnyOf(...)` for one category, `startOver()` for All. (Evolution: tristate → AND s13v → OR/union s13y → radios s13z3 → +All s13z4. Twilight's call each step.) |
| App freezes/hangs when rendering a lone variable tile (e.g. rival/player name code) | **`FontsDB::splice` infinite loop (s13y):** it inserted the expansion but didn't remove the original expandable code, so the code re-expanded forever. Fix: `out.removeAt(position)` before inserting (replace, not insert). Affects `expandStr` → font/name preview rendering. |
| Tileset picker: the very last tile can't be hovered/clicked (off-by-one) | `FontsDB::fontAt(n)` is **1-based** (`getStoreByVal` does `ind--`), so valid indices are `1..fontCount()` **inclusive**. Bounds checks must be `id < 1 \|\| id > fontCount()`, NOT `id >= fontCount()` (that drops the last valid tile). Fixed in `TilesetPicker.qml` s13z2. |
| QML: `property var detailView: null` never fires hover | Parent defines `detailView` but doesn't pass it to child — add `detailView: top.detailView` in child instantiation. |
| QML: `onStrChanged: str = top.str` — str never propagates up | Arrow is backwards. `str = top.str` resets to old value. Should be `top.str = str` to push new value up. |
| QML: `anchors.top: item.Top` — anchor has no effect | Capital `T` in `.Top` is invalid. Use `item.top` (lowercase). |
| `brg.file.data.dataExpanded` = undefined (whole chain) | **REAL root cause (session 13):** these QObject types were `Q_DECLARE_OPAQUE_POINTER`'d (in `savefile_autoport.h` and `area/area.h`), which forces `IsPointerToTypeDerivedFromQObject<T*> = false` → Qt stores them as opaque non-QObject values → QML reads `undefined`. Fix: remove the opaque decl for the QObject type AND fully `#include` its header wherever its pointer is in a Q_PROPERTY/signal/slot/Q_INVOKABLE. `qRegisterMetaType` / `qmlRegisterUncreatableType` do NOT override the opaque decl. |
| `TypeError: Property 'X' of object Y is not a function` | Y.X is a Q_PROPERTY READ (not Q_INVOKABLE). Remove Q_PROPERTY, add Q_INVOKABLE |
| `TypeError: Cannot read property 'player' of undefined` | Property chain traversal fails — see `dataExpanded = undefined` (opaque-pointer) entry above |
| `randomName is not a function` | Should be `randomExample()` (from AbstractRandomString). Check if it's Q_PROPERTY instead of Q_INVOKABLE. |
| `isNaN` / `NaN === NaN` never triggers | `NaN === NaN` is always false. Use `isNaN(x)` |
| `unsigned int` property blank in TextField | Call `.toString()` explicitly in `Component.onCompleted` + Connections handler |
| `parent.width` throws in ListView delegate | `width: parent ? parent.width : 0` |
| Number `TextField` clips its value (shows 1 char / blank) | Width ignored the field padding. Use `width: N * font.pixelSize + leftPadding + rightPadding` (N = max digits). Money/Coins avoid this by using `width: child.implicitWidth`. |
| Stacked Material fields overlap vertically (e.g. Coins on Money) | Layout used `anchors.top: prev.top; topMargin: <fixed>` tuned for shorter Qt 5 fields. Anchor below instead: `anchors.top: prev.bottom; topMargin: 5` — auto-adapts to Qt 6 field height. See qt6-patterns "Material 3 control heights". |
| Number in a `Row` not vertically centered | Don't use `anchors.verticalCenter` inside a Row (ignored). Use `verticalAlignment: TextInput.AlignVCenter` on the field (and remove fixed `topPadding` nudges). |
| `randomName' of object NamesPlayer is not a function` | Method is `randomExample()` (from `AbstractRandomString`). Fixed in `NameDisplayMenuNoTileset.qml` (s10) AND `NameDisplayMenu.qml` (s13e). Grep for other `.randomName(` callers. |
| `TilesetPicker.qml` `Cannot read property 'name' of null` | `brg.fonts.fontAt(id)` can return null even for an in-range id. Guard: `var f = brg.fonts.fontAt(id); if(!f) return; ... f.name`. |
| `Parameter "val" is not declared. Injection ... deprecated` | Qt 6 deprecation, not an error (the signal declares `val`). To silence: `onChangeStr: (val) => {...}` or `function onChangeStr(val) {...}`. |
| Cell/sprite click does nothing but a hover-only button works | The cell `MouseArea` had no `onClicked` — only the hover button did. Add `onClicked` to the `MouseArea` (guard placeholder slots). See `PokemonBoxView.qml` (s13d). |
| Something "works, then breaks after clicking around; reboot fixes it" (fonts/names blank, dropdowns empty, name stops saving) | **QML garbage-collected a parentless C++ object.** DB entries (FontDBEntry, move/species/item entries) live in DB QVectors with no QObject parent → QML treats them as JS-owned and GCs them mid-session → dangling pointers. Fix: call `DB::inst()->qmlProtect(engine)` once at boot (it sets `CppOwnership` on every entry). It existed but wasn't called — wired into `MainWindow::injectIntoQML` (s13f). |
| Crash (read access violation at `0xffff…ffff`) in a model `data()` / `…At()` after clicking an item, esp. after opening+closing its editor | **Q_INVOKABLE returned a parentless QObject → QML gave it JavaScriptOwnership → GC freed it → dangling pointer.** (Q_PROPERTY returns are safe; Q_INVOKABLE returns are NOT.) Fix: wrap the return in `qmlCppOwned()` (`pse-savefile/qmlownership.h`), which sets `CppOwnership`. **Fixed across all 13 savefile `…At()` methods + the 2 model wrappers (s13g/s13h).** Standing rule: any NEW Q_INVOKABLE returning a QObject must be wrapped in `qmlCppOwned()`. See qt6-patterns. |
| Crash in `memcpy`/`memcmp` at `SaveFile::setData` (savefile.cpp) when opening a **recent** file (stack: `openFileRecent` → `setData`) | **Null/short buffer deref.** `FileManagement::readSaveData` returns `nullptr` when `QFile::open` fails (recent path moved/deleted/locked) and `setData` then did `memcpy(this->data, nullptr, …)`. Fixes (s14): (1) `setData` early-returns on a null source; (2) `readSaveData` rejects files whose `file.size()` is **less than** `SAV_DATA_SIZE` (at-least, not exact — larger files load their first 32 KB), returns `nullptr` on failure, and records the **real** technical detail (`QFile::errorString()` for open failures, the actual byte-count for short files); (3) `openFile`/`openFileRecent`/`reopenFile` route through `loadData()` which guards null, emits `loadError()`, and never mutates the live save on failure; (4) startup `pruneRecentFiles()` drops recent entries that can't be opened (**"prune" not "scrub"** — "scrub" already means `wipeUnusedSpace`); (5) a `FileError.qml` modal (router `"fileError"`, raised from App.qml's `Connections` on `brg.file.onLoadError`) shows a plain-English reason centred (`lastErrorMessage`) with the real one-line detail (`lastErrorDetail`) small/muted below — **no made-up error codes**. |
| Save corruption: on every save of a **progressed** file, the last data byte of Box 6 (`0x5A4B`) gets clobbered and the bank-2 box checksum is wrong | **Off-by-one in `recalcChecksums()` (`savefiletoolset.cpp`).** It did `data[0x5A4B] = getChecksum(0x4000, 0x1A4B)`; the bank-2 all-boxes checksum actually lives at **`0x5A4C`** over range **`0x1A4C`** (6 boxes × `0x462`) — per `recalcBoxesChecksums()`, the `savefile-structure.bt` map, and real RBY `sBank2AllBoxesChecksum`. Fix: `data[0x5A4C] = getChecksum(0x4000, 0x1A4C)`. The `if(data[0x284C]==0) return` guard above it (skip box checksums when boxes were never formatted) is **intentional** — it mirrors the game. **Found + verified by `tst_roundtrip` 2026-06-07.** |
| Crash (access violation) in `MapSearch::isType()` during map randomization (`AreaWarps::randomize` → `isType("Cave")`) | `isType()` removed entries whose `toTileset == nullptr` but then **fell through and dereferenced** that null `toTileset` on the very next line. Add `continue;` after the null removal — the sibling `notType()` already guards this way. **Found by `tst_verbs` (randomizer path) 2026-06-07, fixed in `db/.../util/mapsearch.cpp`.** |
| Crash (access violation `0xc0000005`) in `Daycare::~Daycare()` → `QObject::deleteLater` when destroying an **empty** Day Care | `pokemon` is `nullptr` when no mon is deposited (`load()` only allocates it when `getByte(0x2CF4) > 0`; `reset()` leaves it null), but the destructor called `pokemon->deleteLater()` **unconditionally** → null-`this` deref. Masked in the running app (a `SaveFile` is only torn down at process exit, where the crash is swallowed). Fix: guard `if(pokemon != nullptr)` — matches the guard `reset()` already has. **Found by `tst_roundtrip` (2026-06-07), fixed in `daycare.cpp`.** |
| Crash (access violation) in `HoFPokemon::load()` (e.g. via `HoFRecord::randomize()` / `pokemonNew()` / `new HoFPokemon`) | The null check `if(saveFile == nullptr) return;` came **after** `auto toolset = saveFile->toolset;`, so a default-constructed (null-savefile) HoFPokemon dereferenced null. Move the null check **before** the deref (same bug class as `Daycare::~Daycare`). **Found by `tst_randomizer` (full randomize) 2026-06-07, fixed in `hofpokemon.cpp`.** |
| Custom `ComboBox` popup won't scroll — long list clips at screen edge and rubber-bands | The custom `popup` set `implicitHeight: contentItem.implicitHeight` and the inner ListView had `implicitHeight: contentHeight`, so the popup grew to the full list height → ListView height == its content → nothing to flick. Cap it: `height: Math.min(contentItem.implicitHeight + 2, 280)` (`+2` = `padding:1` ×2). Matches Qt's default ComboBox popup. Fixed all 7 `Select*` (s13k). |
| Monochrome SVG icon tinted with `MultiEffect{colorization}` stays dark/black | `colorization` scales the tint by the source's **luminance**; a black SVG (luminance ≈ 0) ⇒ ≈ black regardless of `colorizationColor`. Push it white first: add `brightness: 1.0` before `colorization: 1.0; colorizationColor: <color>`. Fixed the hover pen in `PokemonBoxView.qml` (s13k). |
| QML control "doesn't react" — clicking/changing it does nothing, but Connections-driven value sync still works | Handler written as `function onActivated() {…}` (or `onMoved`/`onClicked`/`onTextChanged`/`onTriggered`/`onCheckedChanged`) **directly on the control** declares an unused method — the signal never calls it. Use property syntax `onActivated: {…}`. (`function onX()` is correct ONLY inside `Connections{}`.) Pervasive in the Pokémon editor, fixed s13l (46 handlers). Tell: working controls used `onX:`, dead ones used `function onX()`. |
| Converting a dead `function onCheckedChanged()` on a CheckBox | Use **`onToggled:`** (user-only), not `onCheckedChanged:` — the latter also fires on the programmatic `checked = …` sync on open and would run side effects (e.g. `makeShiny()` rewriting DV bytes) just from viewing data. Save-integrity rule. |
| ⋮ / icon button renders as a tiny sliver | The icon SVG (e.g. `ellipsis-v`, a tall/narrow glyph) aspect-fits inside `icon.width` × `icon.height`. Setting only `icon.width: 7` leaves `icon.height: 15` (IconButtonSquare default) and the tall SVG fits to a thin sliver. Give it a real square-ish size (`icon.width: 16; icon.height: 16`). Seen on the Future-Shiny ⋮ (s13r). |
| Material control (CheckBox/Button) keeps a Layout row tall despite `Layout.preferredHeight` | The control's implicit/minimum height (~40px touch target) floors the layout. Set `Layout.minimumHeight: 0` on the control (and a `Layout.maximumHeight` on the row) to actually shrink it. |
| QML: `Cannot assign to non-existent property "topInset"` (or similar) after editing a shared component | The component's **root type** changed and lost a property a caller sets. E.g. `NameEdit` went `TextField`→`RowLayout` (s13s), breaking `NameFullEdit` which set `topInset` (s13u). Fix: expose the needed property (`property alias topInset: innerField.topInset`) or revert the root type. Check every instantiation before changing a shared component's root. `NameEdit` is used by `NameFullEdit` (field-only) AND `NameDisplay` (popup). |
| QML ID not accessible in child file | Qt6 strict ID scoping — pass as `property var` through hierarchy |
| `Detected function "onXxx" — no signal matches` | Target object is null/undefined. Fix root cause (property chain), then add null guard: `target: cond ? obj : null` |
| File truncated mid-line | Check brace balance: `text.count('{') - text.count('}')`. Rewrite from memory if short. |
| App hangs 40 seconds on startup | Likely MSAA or `qt_add_qml_module()` conflict. See decisions/architecture.md |
| Window never appears | `QQuickWidget::setSource()` hung. See decisions/architecture.md |
| Footer button rounded/shadowed in Qt 6.5+ | `flat: true` + `topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0` |
| `IconButtonSquare` renders rounded/shadowed | It extended `RoundButton` via `IconButtonRound`. Change to `Button { flat: true; topInset:0; bottomInset:0; leftInset:0; rightInset:0 }` |
| `randomName is not a function` | Correct method is `randomExample()` (from `AbstractRandomString`) — not `randomName` |
| `brg.file.data.dataExpanded` = undefined | **Superseded (session 13).** rebuild + `qRegisterMetaType` did NOT fix it. Real cause is `Q_DECLARE_OPAQUE_POINTER` forcing the chain types to be treated as non-QObjects — see the opaque-pointer entry above. |
| Many QML components show blank data after fresh build | Source files may be truncated. Run brace-balance check across all `.qml`/`.cpp`/`.h`. Repair via byte-level Python append. |
| `unknown type name 'FontFilter'` in fontsdb.h | `FontFilter` was a private type lost in truncation. Replace with `int` in private `splice()` declaration. |
| `no member named 'getMapCount'/'isCity'/'notCity' in 'MapSearch'` | Moc was generated from old complete header. Add method declarations matching Q_PROPERTY READ specs. |

| DB header: `use of undeclared identifier 'store'` / `'ind'` | Private data members were truncated out of the header. Add `QVector<XxxDBEntry*> store;` and `QHash<QString, XxxDBEntry*> ind;` back to the `private:` section. |
| DB header: `int buyPrice() const` as private method, `.cpp` uses it as data member | Method declaration was wrong (truncation). Replace `int buyPrice() const; int sellPrice() const;` with `int buyPrice = 0;` data member. `sellPrice` is computed, not stored. |
| `'buyPrice' is a private member` + `expression is not assignable` | Both: header declared it as a method, not a field. Fix header first, then add `friend struct GameCornerDBEntry;` to `GameCornerDB`. |
| `'toGameCorner' is a protected member of 'ItemDBEntry'` | Add `friend struct GameCornerDBEntry;` to `ItemDBEntry`'s friend list (alongside `ItemsDB` and `MapDBEntrySpriteItem`). |
| `'toEvolvePokemon' is a protected member of 'ItemDBEntry'` | Add `friend struct PokemonDBEntryEvolution;` to `ItemDBEntry`'s friend list. |
| `'toTeachPokemon' is a protected member of 'ItemDBEntry'` | Add `friend struct PokemonDBEntry;` to `ItemDBEntry`'s friend list. |
| `undefined symbol: DB::qmlProtect` / `DB::qmlHook` | Declared as public slots → MOC always generates dispatch entries. Must add bodies in `db.cpp` even if not yet called. |
| `definition of implicitly declared default constructor` | Header is missing the private constructor declaration. Compiler synthesized one; `.cpp`'s explicit definition then conflicts. Add `private: XxxDB();` to the header. |
| `out-of-line definition of 'getMaps' does not match any declaration` | Method implemented in `.cpp` but not declared in header (truncation). Add matching declaration to header. |
| `no matching constructor for initialization of 'MapSelectModel'` | Default member initializer `= new MapSelectModel` has no matching ctor. Real init is in the constructor's mem-init-list in `.cpp`. Change to `= nullptr`. |
| `mapData->isOutdoor()` — no member named 'isOutdoor' in 'MapDBEntry' | `MapDBEntry` has no such method. The concept lives on `TilesetDBEntry`: `mapData->getToTileset()->typeAsEnum() == TilesetType::OUTDOOR`. |
| `ignoring return value of function declared with 'nodiscard'` on `DB::inst()` | Call is for side effects (bootstrapping). Use `(void)DB::inst();` to suppress. |
| `ignoring return value` on `QFile::open()` | Qt 6 marked `QFile::open()` `[[nodiscard]]`. Wrap: `if(!file.open(...)) return;` — also improves correctness. |

## Undeclared Identifiers (Renamed Constants/Types)

| Error | Fix |
|-------|-----|

