# System Map {#pse_systems_about}

A deep, structured account of how Pokered Save Editor 2 is built -- the macro
systems, the micro systems, and how they wire together. This is the "understand
the whole machine" reference; it grows alongside the project-wide documentation
pass tracked in [../reference/documentation.md](../reference/documentation.md).

Read in this order:

| Doc | Scope |
|-----|-------|
| [overview.md](overview.md) | **Start here.** The macro picture: the four layers, the boot sequence, the runtime data flow, and the byte-fidelity contract. |
| [common.md](common.md) | The `common` layer -- shared integer types, Random, Utility, the QML-ownership pattern. |
| [savefile.md](savefile.md) | The byte-exact parse/expand/flatten core (core spine done; `expanded/` tree in progress). |
| [db.md](db.md) | The singleton reference databases and entry structs (spine + conventions done; databases in progress). |
| app.md | _(written as the app layer is documented)_ Boot, Bridge, engine providers, QML list models. |
| [qml.md](qml.md) | The QML screen/fragment tree and how it binds to the Bridge (convention set; components in progress). |

How this relates to the other notes:

- [../context/architecture.md](../context/architecture.md) is the quick orientation; this folder is the in-depth version.
- [../decisions/architecture.md](../decisions/architecture.md) explains *why* key structures are the way they are.
- [../reference/qt-patterns.md](../reference/qt-patterns.md) holds the Qt 6 mechanics referenced throughout.
