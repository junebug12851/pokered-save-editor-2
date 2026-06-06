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
| `QFileDialog` not found | Add `Qt6::Widgets` to CMakeLists target_link_libraries |
| `Q_DECLARE_OPAQUE_POINTER` parse error | Add `#include <QMetaType>` before the macro |
| `convertToCode` returns `QVector<int>`, need `QVector<var8>` | Cast element-by-element |
| `GameCornerDB::buyPrice` private | `GameCornerDB::inst()->getBuyPrice()` |
| DB constructor static-init deadlock | Remove `load()` from DB constructor — `DB::loadAll()` calls it |

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
| `randomName is not a function` | Should be `randomExample(