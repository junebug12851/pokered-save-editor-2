# System Map: `app` layer

The Qt executable (`projects/app/src`) -- the boot sequence, the QML<->C++ bridge,
image/engine providers, and the Qt item-models that feed the QML views. The QML UI
itself lives in `projects/app/ui` (documented separately; see
[../reference/documentation.md](../reference/documentation.md) for why QML isn't in
the generated docs). See the macro picture in [overview.md](overview.md).

## Shape

| Part | What |
|------|------|
| `main.cpp` | Entry point -- `return boot(argc, argv)->exec();`. |
| `boot/` | The bootstrap: `boot.cpp` (sequence), `bootDatabase.cpp` (`DB::inst()`), `bootQmlLinkage.cpp` (register every type with QML). |
| `bridge/` | `Bridge` (the `brg` aggregate), `Router` (screen nav), `Settings` (theme/layout). |
| `engine/` | QQuickImageProviders + helpers that render font/tileset previews for QML. |
| `mvc/` | ~17 `QAbstractListModel` subclasses adapting C++ data into list/table form for QML views. |

## Boot sequence

`main()` -> `boot()` runs, in order: `bootDatabase()` (brings up the whole `db`
layer), `bootQmlLinkage()` (registers every C++ type/enum with QML -- long and
deliberately repetitive because Qt's meta-system rejects helpers there),
`Router::loadScreens()` (registers the screen set), then `createApp()` which
creates the QApplication and shows the MainWindow. Full detail in
[overview.md](overview.md).

## The Bridge pattern (`brg`)

`Bridge` is injected into the QML context as **`brg`** and is the single doorway
between QML and C++. Off it hang:

- `brg.file` -> FileManagement -> the `data.dataExpanded.*` editable save tree;
- `brg.router` (navigation) and `brg.settings` (theme/colour palette/layout);
- the databases/randomizers QML uses directly (`fonts`, `randomPlayerName`,
  `randomExample*`, `util`);
- a fleet of **list models** (`pokedexModel`, `mapSelectModel`, `marketModel`,
  `pokemonStorageModel*`, the `*SelectModel` pickers, ...).

For QML to traverse `brg.file.data.dataExpanded.*`, every QObject in that chain
must be a complete type at the property's MOC TU -- the rule documented in
[savefile.md](savefile.md) and [../reference/qt-patterns.md](../reference/qt-patterns.md).

## Router and Settings

- **Router** holds a registry of named `Screen`s and the live navigation `stack`;
  QML calls `changeScreen()`/`closeScreen()` and the Router emits nav signals the
  QML shell acts on.
- **Settings** is the single source of truth for theming: header metrics, the
  Material colour palette, the per-font-category keyboard colours, and the
  name-preview tileset.

## engine/ and mvc/

- **engine/** -- image providers (font preview, tileset) that render Game Boy
  graphics on demand for QML `Image` sources, plus the tileset engine behind name
  previews.
- **mvc/** -- the C++ side of Qt's model/view: each model wraps a C++ collection
  (DB entries or save objects) as a `QAbstractListModel` so a QML `ListView`/picker
  can show it. Because Qt can't hand a raw vector to QML, these models (and the
  `...Size` + `...At()` accessors on the DB entries) are how lists reach the UI.

> **Note -- MainWindow:** the QQuickWidget host `MainWindow` is C++ but lives under
> `projects/app/ui/window` (with the QML). It is documented and **now included in the
> Doxygen docs** via a dedicated `projects/app/ui/window` entry in the Doxyfile INPUT
> (the QML itself, `app/ui/app`, stays out). This is the one C++ class outside
> `app/src`.

> Documentation status: boot + bridge spine done (main, boot/*, bridge.h, router.h,
> settings.h). `engine/`, `mvc/`, and the `.cpp` are in progress -- tracked in
> [../reference/documentation.md](../reference/documentation.md).
