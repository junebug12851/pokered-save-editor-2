# Debug Automation Harness & Fast-Dev Loop

## `hover` — the pointer, no button (added 2026-07-17)

```json
{"cmd":"hover","x":400,"y":300}      // or {"cmd":"hover","obj":"someItem"}
```

Drives `MouseArea.containsMouse`, `HoverHandler.hovered` and every `onEntered` **exactly as a real
cursor does** — because that is all a hover is: a `QEvent::MouseMove` with **no buttons held**.
Everything the map screen does under the pointer (the cell outline, the tab strip appearing, a tab
lighting its own box, a tooltip) is driven by this and nothing else, so without it none of it could
be checked without a human sitting there. Verified live: one `hover` updates the status bar's
`tile 11, 15 · block 5, 7` and pops a warp's tooltip.

**It exists because I claimed hover was untestable and project leadership refused to accept it** — *"i find
it hard to beleive theres no solution for testing hover you cannot tell me the community has no
solution for this"*. They were right; it took two lines. Same failure as the aqtinstall "hard ceiling"
the same day: I hit the edge of what I had **already built** (`tap` sends press+release), and
reported the *capability* as missing. **"My harness can't" is not "it can't."**
See [`qt-patterns.md`](qt-patterns.md) (top).

The DEBUG-only automation harness (added 2026-07-09, `feat(app): debug-only automation harness`) turns
manual iteration into a fast, hands-off loop: **launch straight to the screen you're editing with a save
already loaded, then preview QML edits live via hot-reload** — no clicking through the New File modal, no
relaunch on every tweak. All of it is gated behind `QT_DEBUG`; **release builds don't contain any of it**
(the servers/flags compile to no-ops).

Sources: `projects/app/src/boot/debuglaunch.cpp` (launch flags), `projects/app/src/boot/debugserver.cpp`
(TCP control channel), `projects/app/ui/window/mainwindow.cpp` (`setupHotReload`, `reloadQml`,
`saveShot`, the `QmlDiskInterceptor`).

## The fast-dev default loop (use this by default for UI work)

For any QML/screen work, prefer this over the rebuild-relaunch cycle:

1. **Launch once, straight to the target screen, with the default save loaded and hot-reload on:**
   ```
   PokeredSaveEditor.exe --hot --sav <default.sav> --screen <name> [--select party:N]
   ```
   Default save fixture: `assets/saves/natural-clean/BaseSAV.sav` (the same populated fixture the
   screenshooter uses). `<name>` ∈ the registered screens (trainerCard, pokedex, bag, pokemart, pokemon,
   rival, maps, home, pokemonDetails, …). (Launch still needs the Qt `bin` on PATH — see `collaboration.md`.)
2. **Edit QML** — the file watcher reloads live (see hot-reload below); no relaunch needed.
3. **Only rebuild + relaunch when C++ / `.qrc` / a NEW `.qml` file changes** (a new QML file must be added
   to `app/app.qrc`, which is a build input; existing-file edits hot-reload). C++ changes need the kit-dir
   rebuild per the normal loop.
4. **Screenshot review without stealing focus:** grab the running app with `--shot <png>` at launch, or
   the `shot` TCP command any time — `MainWindow::saveShot` uses `QQuickWidget::grabFramebuffer`, so it
   works while the window is **backgrounded / occluded / minimized** (no focus juggling). This is the
   preferred way to do the mandatory manual screenshot review during live iteration; the headless
   `screenshooter` (see `screenshots.md`) remains the batch/CI path.

## Launch flags (`debuglaunch.cpp`, all optional, DEBUG-only)

| Flag | Effect |
|------|--------|
| `--sav <path>` / `--save <path>` | Load this `.sav` on startup (`setPath` + `reopenFile`). |
| `--screen <name>` | After load, dismiss the startup New File modal and navigate to `<name>`. |
| `--select <spec>` | Selection for screens that need one — `party:N` for `pokemonDetails` (opens party mon N). `random` / `source:id` reserved (not fully wired). |
| `--hot` | Enable live QML hot-reload (load QML from the source tree + watch it). |
| `--shot <path>` | After navigating (≈600 ms settle), grab the view to this PNG. Works backgrounded. |
| `--minimized` | Start minimized (drive/screenshot without the window popping up / stealing focus). |

## Live control channel (`debugserver.cpp`) — `127.0.0.1:8766`

A newline-delimited-JSON TCP server (DEBUG-only, GUI thread, safe to touch QML/widgets). Send one JSON
object per line, get one reply per line. Verbs:

- `{"cmd":"ping"}` → `pong`
- `{"cmd":"screen","arg":"trainerCard"}` — navigate; reply = new title. (`pokemonDetails` takes `"index"`.)
- `{"cmd":"party","arg":N}` — open party mon N's details; `{"cmd":"back"}` / `{"cmd":"home"}`.
- `{"cmd":"sav","arg":"C:/…/x.sav"}` — load a save.
- `{"cmd":"shot","arg":"C:/…/out.png"}` — grab the view to a PNG (backgrounded-safe).
- `{"cmd":"title"}` — current screen title.
- `{"cmd":"get","obj":"<objectName>","prop":"<p>"}` / `{"cmd":"set","obj":…,"prop":…,"val":…}` — read/write a QML property.
- `{"cmd":"click","obj":"<objectName>"}` — emit a control's `clicked()` **signal**.
- `{"cmd":"tap","x":152,"y":294}` / `{"cmd":"tap","obj":"<objectName>"}` — a **REAL mouse
  press+release**, through Qt's actual delivery path.

  ⚠️ **`click` and `tap` are not the same thing, and the difference is a whole class of bug.**
  `click` emits the signal directly and therefore exercises **none** of the delivery machinery — no
  grabs, no pointer handlers, no propagation. If a bug is about *which item gets the event* (who
  grabs it, who consumes it, what it falls through to), `click` **cannot see it** and `tap` can.

  That gap is why the "opening the picture picker drops your selection" bug survived a fix and why I
  ended up clicking the screen by hand instead of using our own tooling. Project leadership asked why; the
  answer was that the harness couldn't do it. Now it can. **Reach for `tap` whenever the question is
  "what happens when the user actually clicks *there*".**
- `{"cmd":"reload"}` — force a QML reload. `{"cmd":"list","arg":"<substr>"}` — matching `objectName`s.

> ✅ **Everything named is reachable — including `Repeater`/model-built controls (fixed 2026-07-15).**
> `list`/`get`/`set`/`click`/`tap`-by-object resolve names over the **full** tree: the QObject
> parent-child tree **and** the Qt Quick visual tree (`QQuickItem::childItems()`). This matters because
> a `Repeater` or model delegate — every **dock rail button** (`dockBtn_<id>`), every list row, anything
> built from an array — lives in the *visual* tree but is often **not** returned by
> `QObject::findChildren`. Before the fix, hand-declared items (`mapRightPanel`) were reachable while
> model-built ones (`dockBtn_charstate`) silently were not, so you couldn't `click` a rail button by
> name and fell back to guessing `tap` coordinates. The promise is universal: **if it has an
> `objectName`, you can target it.** Name convention: give the control a stable name from its id/key or
> array index (the rail does `objectName: "dockBtn_" + modelData.id`) so the handle is predictable.
> (`debugserver.cpp` → `collectTree`/`reachableObjects`.)

**Navigate by PATH, and reach even UNNAMED nodes (2026-07-15).** Any `obj` field (in `get`/`set`/
`click`/`tap`/`invoke`/`shot`) accepts a **`/`-separated path**: the first segment is a named node
(any `objectName`, or `root`), each following segment steps **down one level** — a **number** is an
index into that node's direct children (visual order first, so it matches what you see), a **word** is
a child's `objectName`. So you can start at a named ancestor and walk to anything, named or not:
`get obj=mapCharacterState/0 prop=width` → the panel's ScrollView. To see what's under a node,
`list` it: `{"cmd":"list","obj":"mapRightPanel"}` → `["[0] Type name", "[1] …", …]` — pick an index,
append it to the path.

**Trigger anything (2026-07-15).**
- `{"cmd":"invoke","obj":…,"method":"toggle","args":[…]}` — emit **any signal** or call **any**
  slot/`Q_INVOKABLE` by name (no-arg is the always-works case: fire a signal/event; args are passed as
  QVariant, so typed-param QML slots may not match — prefer `var`/no-arg).
- `{"cmd":"tap",…,"double":true}` — a real **double-click**; `"button":"right"|"middle"` for the
  other buttons (right-click menus drive too). `"clicks":2` is an alias for `double`.

**Screenshot from any component down (2026-07-15).** `{"cmd":"shot","arg":"out.png","obj":"<name|path>"}`
crops the grab to that component's bounds — grab one panel/control with **no manual cropping**. Omit
`obj` for the whole view. (`mainwindow.cpp` → `saveShot` maps the item's scene rect to physical pixels
via the framebuffer/widget ratio, so it's correct under a device-pixel-ratio.)

Drive it from PowerShell with a `TcpClient` (write a line, read a line). Great for scripted checks:
navigate, `set` a field, `shot`, then eyeball.

### The dev MCP server drives all of this now (2026-07-16)

For AI sessions, the standard way to drive the harness (and builds, tests, PyBoy) is the
**dev MCP server** — [`dev-mcp.md`](dev-mcp.md). It encodes every trap below (fresh connection
per command, settle-aware shots) so they can't recur, and returns screenshots inline as images.

### ⚠️ Two traps that will eat an hour each (found 2026-07-13, the hard way)

**1. ~~Do NOT `screen` to a screen you are already on~~ — FIXED AT THE SOURCE (2026-07-16).** The
router *pushes* — it does not check — so navigating to the screen you were already on used to stack a
**dead duplicate** behind the live one: every `set` replied `{"ok":true}` onto the invisible copy, every
`get` read its stale value, every `shot` looked unchanged. **The `screen` verb now refuses the duplicate
push itself** (reply carries `"already": true` and nothing moves; `pokemonDetails` is exempt — re-navigating
there switches the open mon). Two related upgrades landed with it: **`title` now also returns the
registered screen NAME** (`{"result":"Map","screen":"maps"}` — compare against what you navigate by), and
**navigation verbs (`screen`/`party`/`back`/`home`) reply only after the stack transition has finished**
(they poll the two StackViews' `busy` via their new `appRoot`/`appBody` objectNames, GUI kept running).
The old tell — `list` returning the same name three times — still works if a stack ever gets dirty.

**2. One connection per command.** Hold a single socket open for a whole scripted run and every `shot`
comes back **byte-identical** — the grab returns the same frame no matter what changed in between.
Open a fresh `TcpClient` per command (it costs nothing) and the grabs are live. A working driver script
is `tmp/drive.ps1`'s shape:

```powershell
function Send($obj) {
  $c = New-Object System.Net.Sockets.TcpClient('127.0.0.1', 8766)
  $s = $c.GetStream(); $w = New-Object System.IO.StreamWriter($s); $w.AutoFlush = $true
  $r = New-Object System.IO.StreamReader($s)
  $w.WriteLine(($obj | ConvertTo-Json -Compress))
  $reply = $r.ReadLine(); $c.Close(); return $reply
}
```

**And a third, smaller one:** grab shots with `QT_QPA_PLATFORM=offscreen`, **not** with a *minimised*
window. A minimised window stops rendering, so `shot` hands back whatever was on screen when it went
down. (Offscreen loses the pixel-art shader — `PixelImage` falls back to nearest — which does not
matter for a *layout* review, which is what the mandatory pass is.) Set `QT_QPA_FONTDIR=C:\Windows\Fonts`
or every string renders as tofu boxes.

**A fourth, fixed: "distorted" shots were MID-TRANSITION grabs (2026-07-16).** A `shot` fired right
after a navigation caught the screen half-faded (the New File modal dissolving over the Map screen looks
like a wrecked composite). Two-part fix in `debugserver.cpp`: `shot` always **waits out any in-flight
stack transition** first, and takes an optional **`"settle"` ms** (bounded event-processing pause, capped
5 s) for other animations — `{"cmd":"shot","arg":"x.png","settle":250}`. Navigation verbs settling before
they reply (trap #1 box) closes the other half of the window.

## How hot-reload works (and its known limitation)

`--hot` installs a `QmlDiskInterceptor` (a `QQmlAbstractUrlInterceptor`) that redirects `qrc:/…qml|js`
URLs to the matching file on the **source tree** (`PSE_QML_SOURCE_DIR`), so the running app loads QML from
disk instead of the baked qrc. A `QFileSystemWatcher` over `<root>/ui/**/*.qml|*.js` fires on save
(debounced 160 ms; re-arms the watch because editors rewrite/rename on save) and calls `reloadQml()`.

`reloadQml()` = `engine->clearComponentCache()` + tear down and re-`setSource(App.qml)` — i.e. it rebuilds
the **whole** QML tree from the root. **Per-file partial reload is deliberately NOT attempted** — in a
single-`QQuickWidget` app the engine caches whole compiled components; reloading just the changed file
would leave consumers of a shared component holding a stale cached copy (a subtle "quality loss" the
project forbids). So the reload is always full and reliable.

**It preserves context (2026-07-10 fix).** A full-tree reload otherwise dumps you back at Home + the New
File modal. `reloadQml()` now, before tearing down, records the **current screen** (reverse-looks-up the
top of `Router::stack` to its registered name); after the reload it polls until App.qml re-seats its
startup stack, dismisses the New File modal, and **navigates back to that screen** (`pokemonDetails`
re-opens party mon 0, since it needs a selection). The **loaded save survives automatically** — it lives
in C++ (`Bridge`/`FileManagement`), not QML, so only navigation needs restoring. Net effect: edit a
screen's QML, save, and you stay on that screen with your save loaded, changes applied. (Verified over the
TCP channel: `title` → `reload` → `title` returns the same screen; Home/modals are intentionally not
auto-restored.)
