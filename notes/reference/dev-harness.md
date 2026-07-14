# Debug Automation Harness & Fast-Dev Loop

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

  🔴 **`click` and `tap` are not the same thing, and the difference is a whole class of bug.**
  `click` emits the signal directly and therefore exercises **none** of the delivery machinery — no
  grabs, no pointer handlers, no propagation. If a bug is about *which item gets the event* (who
  grabs it, who consumes it, what it falls through to), `click` **cannot see it** and `tap` can.

  That gap is why the "opening the picture picker drops your selection" bug survived a fix and why I
  ended up clicking the screen by hand instead of using our own tooling. Twilight asked why; the
  answer was that the harness couldn't do it. Now it can. **Reach for `tap` whenever the question is
  "what happens when the user actually clicks *there*".**
- `{"cmd":"reload"}` — force a QML reload. `{"cmd":"list","arg":"<substr>"}` — matching `objectName`s.

Drive it from PowerShell with a `TcpClient` (write a line, read a line). Great for scripted checks:
navigate, `set` a field, `shot`, then eyeball.

### ⚠️ Two traps that will eat an hour each (found 2026-07-13, the hard way)

**1. Do NOT `screen` to a screen you are already on.** The router *pushes* — it does not check. Launch
with `--screen maps` and then send `{"cmd":"screen","arg":"maps"}` and there are now **two Map screens**
on the stack, one dead behind the other. `findItem` walks the object tree and returns the **first**
match, which is the dead one. So:

* every `set` replies `{"ok":true}` — it really did set the property, on an invisible copy;
* every `get` reads that copy's stale value;
* every `shot` looks completely unchanged, no matter what you do.

The tell is `{"cmd":"list","arg":"mapLeftPanel"}` coming back with the **same name three times**. If a
`set` "works" and the screen doesn't move, `list` your object before you debug anything else.

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
