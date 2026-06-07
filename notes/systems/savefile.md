# System Map: `savefile` layer

The byte-exact engine (`projects/savefile/src/pse-savefile`). It loads a raw Gen 1
save, expands it into editable objects, and flattens it back -- touching only the
bytes an edit requires. It knows nothing about the UI. See the macro picture in
[overview.md](overview.md).

> For the **save-format facts themselves** -- byte offsets, the Gen 1 checksum algorithm, the
> badge bitfield, VRAM pointers, 16-bit endianness, and the gameplay quirks this layer encodes
> (retroactive natures/shininess, trade status, level-<5 underflow, …) -- see
> [../reference/gen1-knowledge.md](../reference/gen1-knowledge.md). This file is the *code map*;
> that one is the *domain knowledge*.

> Documentation status: **all 50 headers are documented + verified** -- the core
> spine *and* the entire `expanded/*` object tree (root, player, fragments, area,
> world, root siblings). Only the `.cpp` implementations remain. Tracked in
> [../reference/documentation-progress.md](../reference/documentation-progress.md).

## The core spine

| File | Role |
|------|------|
| `savefile.h` / `.cpp` | **SaveFile** -- the hinge. Holds the raw 32 KB `data`, the `dataExpanded` object tree, and a `toolset`. Verbs: `expandData()`, `flattenData()`, `resetData()`, `eraseExpansion()`, `randomizeExpansion()`. |
| `savefiletoolset.h` / `.cpp` | **SaveFileToolset** -- address-based byte primitives that speak the save's encodings: BCD numbers, font-encoded strings, bit-fields, words, and the Gen 1 checksum. |
| `savefileiterator.h` / `.cpp` | **SaveFileIterator** -- a moving cursor over the toolset. Same primitives, but address-free: each call reads/writes at `offset` and auto-advances. push()/pop() bookmark stack. Used to walk the save during expand/flatten. |
| `filemanagement.h` / `.cpp` | **FileManagement** -- the I/O + file-lifecycle controller QML sees as `brg.file`: new/open/save/reopen, recent-files list (QSettings), disk read/write. |
| `savefile_autoport.h` | The library's import/export macro **and** the central list of save-tree QObject types kept deliberately opaque to QML (build-speed optimisation). |
| `qmlownership.h` | `qmlCppOwned()` -- pins a Q_INVOKABLE-returned QObject to C++ ownership so QML's GC can't free it (the other half of the GC-crash fix; the first half is `Utility::qmlProtectUtil` in [common.md](common.md)). |

### How the bytes flow

```
FileManagement.readSaveData(path)  ->  SaveFile.data (raw var8*, 32 KB)
SaveFile.expandData()              ->  walks data via a SaveFileIterator,
                                       building the SaveFileExpanded tree
   (QML edits the tree through Q_PROPERTY bindings)
SaveFile.flattenData()             ->  writes the tree back through the toolset,
                                       ONLY the strictly-necessary bytes
FileManagement.writeSaveData(path) <-  SaveFile.data
```

`SaveFileToolset::recalcChecksums()` follows the same byte-fidelity discipline:
it only writes the checksums the real game would actually compute (e.g. it skips
the box checksums in the cases where the game never calculates them). See the
"critical rule" comment on `SaveFile::flattenData()` and
[../context/principles.md](../context/principles.md) -> "Save File Integrity Is
Sacred".

## The expanded object tree (`expanded/`)  _(documentation in progress)_

`SaveFileExpanded` is the root of the editable tree; its sub-objects mirror the
save's regions. Grouped on disk as:

- `expanded/player/` -- PlayerBasics, PlayerPokedex, PlayerPokemon (party), etc.
- `expanded/area/` -- the current map/area: AreaMap, AreaGeneral, AreaWarps,
  AreaSprites, AreaTileset, AreaAudio, ...
- `expanded/world/` -- world state: events, missables, scripts, towns, trades,
  completion, ...
- `expanded/fragments/` -- reusable pieces: PokemonBox/Party, Item, ItemStorageBox,
  SignData, WarpData, SpriteData, HoF records, ...

Each of these is a QObject exposing Q_PROPERTY leaves the UI binds to. The ones QML
traverses are fully `#include`d (complete types) so the property chain resolves;
the ones it doesn't are kept opaque in `savefile_autoport.h`. Per-object detail
will be filled in here as the `expanded/` headers are documented.
