# System Map: QML UI layer

The user interface (`projects/app/ui/app`) -- 95 `.qml` files, ~7.8k lines. This is
the front end the player actually touches; it binds to C++ entirely through the
`brg` aggregate (see [app.md](app.md)). See the macro picture in
[overview.md](overview.md).

> **Not Doxygen-generated.** Per the documentation decision
> ([../decisions/architecture.md](../decisions/architecture.md)), the QML is
> documented with plain human comments in-source, not via a generator (qdoc was
> rejected on UX grounds). These notes + the in-file comments are the QML docs.

## Structure

```
App.qml                     the QML root: a StackView (body + modal pages)
  sections/AppWindow.qml     the always-present app body (header + page area)
  screens/
    non-modal/               the main pages: Home, TrainerCard, Bag, Pokedex,
                             Pokemon, PokemonDetails, Maps, MapDetails, Pokemart, Rival
    modal/                   full-window modals (e.g. NewFile)
  fragments/                 reusable components, grouped by area:
    general/                 shared widgets (buttons, text edits, tooltips, NameDisplay, ...)
    header/                  the app header bar pieces
    modal/                   modal scaffolding
    controls/                editing controls:
      name/ name-full/        the name editor + full on-screen keyboard
      selection/              pickers (species/move/status/... combo popups)
      menu/                   the kebab (vertical-dots) menus
    screens/                 screen-specific fragments:
      trainer-card/ pokemon/ pokemon-details/(stats/) bag/ home/
```

## How it binds to C++

Everything flows through the single context property **`brg`** (the Bridge):

- **Data** -- `brg.file.data.dataExpanded.*` is the editable save tree; fields are
  two-way-bound to controls.
- **Navigation** -- `brg.router` (App.qml turns its signals into StackView push/pop).
- **Theme** -- `brg.settings` (colours, header metrics, font-category colours).
- **Lists** -- the `brg.*Model` list models feed `ListView`/pickers.
- **Graphics** -- `image://` sources resolve through the C++ image providers
  (font preview, tileset) in `app/src/engine`.

The Qt 6 rule that makes the `brg.file.data.dataExpanded.*` chain readable from QML
(complete types, not opaque) is in [savefile.md](savefile.md) /
[../reference/qt6-patterns.md](../reference/qt6-patterns.md). UI conventions
(layouts, borderless combos, the kebab buttons, editor popups, sliders) are in
[../reference/ui-patterns.md](../reference/ui-patterns.md) -- read that before UI work.

## Comment convention for QML

QML files start with `import` lines (no licence header). The house style:

- A **file-header `//` block at the very top** (before the imports) stating what the
  component is, what it binds to, and any non-obvious behaviour.
- Inline `//` comments on non-obvious properties, signals, JS functions, and layout
  tricks (especially the Qt 6 Material height workarounds noted in `ui-patterns.md`).
- **Preserve every existing comment** (the maintainer iterated these heavily); merge, never
  delete. Comments only -- never change UI code while documenting.

> Documentation status: `App.qml` done (the convention setter). Screens, sections,
> and fragments are in progress -- tracked in
> [../reference/documentation-progress.md](../reference/documentation-progress.md).
