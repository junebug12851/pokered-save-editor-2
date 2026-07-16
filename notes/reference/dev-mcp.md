# The dev MCP server — one reliable transport for the whole dev loop

`scripts/mcp/pse_dev_mcp.py` (launcher `scripts/mcp/run.cmd`, venv `tmp/mcp-venv` via
`scripts/mcp/setup.ps1`) is an MCP (Model Context Protocol) server that gives an AI
session — Claude/Cowork — a **standardized, reliable, leak-proof** way to drive the
entire rapid-development workflow: configure/build/clean-rebuild (both build dirs),
run tests, launch and drive the app through the DEBUG harness, take screenshots (app
and emulator, returned inline as images), forge saves, and drive PyBoy interactively
or single-shot — including installing/updating/health-checking PyBoy itself.

**Why it exists (2026-07-16).** The 2026-07-15 flag-scenario session wedged the
machine: PyBoy driven through the interactive shell's ~44 s per-call transport leaked
6–10 processes, starved the CPU, and every later call timed out — a feedback loop
(diagnosed in [`emulator-verification.md`](emulator-verification.md) → "Flag-scenario
test framework"). The problem was never PyBoy or the test logic; it was
**orchestration through a transport with a hard per-call cap and no process
ownership**. An MCP server has neither flaw: it is a long-lived local process that
**owns every child it spawns**, applies real timeouts, and kills whole process trees
on any overrun.

## The rules it encodes (all four are project law)

1. **Background by default; foreground is an explicit act.** Builds, tests, emu runs
   are hidden console-less children logged to `tmp/mcp-jobs/`. `app_launch` defaults
   to a minimized window (or fully headless `mode='offscreen'`); **`app_foreground()`**
   is the deliberate "it's ready for her to look at" moment (the standing 2026-07-12
   rule), and `app_background()` puts it away again.
2. **Jobs, never blocking calls.** Anything long returns a `job_id` immediately;
   `job_wait`/`job_status`/`job_log` poll it; a job that overruns its timeout has its
   **entire process tree killed** (`taskkill /T /F`). The server kills all live jobs
   and sessions at shutdown, and `procs_cleanup()` sweeps strays (the exact leak class
   of 2026-07-15). Crash-fast error mode (`SetErrorMode(0x0003)`) is set in the server
   and inherited by every child, so a crashing exe exits instead of hanging on a
   debugger dialog.
3. **One PyBoy per process — always.** PyBoy wedges if re-instantiated in one process
   (proven 2026-07-15). Single-shot probes stay single-shot (`emu_run_script`);
   the batch runs **one scenario per child** (`emu_flag_scenarios`); and interactive
   driving is a fresh owned child REPL per boot: `scripts/emu/drive_session.py`,
   NDJSON over stdin/stdout, every loop frame-bounded, EOF ⇒ clean stop, parent
   timeout ⇒ tree-kill. (This replaces the abandoned file-IPC `emu_server.py`
   experiment — right idea, wrong transport/ownership.)
4. **The exact kit toolchain, verbatim.** Jobs run with
   `C:\Qt\Tools\llvm-mingw1706_64\bin;C:\Qt\6.11.0\llvm-mingw_64\bin;C:\Qt\Tools\CMake_64\bin`
   prepended to PATH and `CC=clang CXX=clang++`. The **kit dir**
   (`projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`, Makefiles — what the
   app runs from) and the **test build** (`build/`, Ninja) are separate tools and
   never mixed up. Headless children get `QT_QPA_PLATFORM=offscreen` +
   `QT_QPA_FONTDIR=C:\Windows\Fonts`.

## Tool map

| Area | Tools | Notes |
|---|---|---|
| Info | `workspace_info` | VERSION, branch/HEAD, what exists, live jobs/sessions |
| Build | `build_app` · `build_tests` · `configure_tests` | kit dir vs `build/`; `clean` = `--clean-first`; jobs |
| Test | `run_ctest` · `run_test_exe` | offscreen + fonts + crash-fast; jobs |
| Shots | `capture_screenshots` | the headless screenshooter batch (`screenshots.md`) |
| Jobs | `job_status` · `job_wait` · `job_log` · `job_kill` · `job_list` | logs under `tmp/mcp-jobs/` |
| App | `app_launch` · `app_stop` · `app_foreground` · `app_background` · `app_cmd` · `app_screen` · `app_title` · `app_load_sav` · `app_get` · `app_set` · `app_click` · `app_tap` · `app_invoke` · `app_list` · `app_reload` · `app_shot` | the DEBUG TCP harness (`dev-harness.md`), traps encoded (below) |
| Emu | `emu_status` · `emu_setup` · `emu_check_updates` · `emu_update` · `emu_run_script` · `emu_flag_scenarios` · `emu_forge_save` · `emu_boot` · `emu_button` · `emu_tick` · `emu_mem` · `emu_poke` · `emu_state` · `emu_screenshot` · `emu_stop` | ROM-gated, local-only; clean unavailability without it |
| Hygiene | `procs_cleanup` | strays list/kill; `include_app` for orphan editors |

`app_shot` and `emu_screenshot` return the PNG **inline as an MCP image** (and save a
copy under `tmp/mcp-shots/`), so the reviewing session sees the pixels directly — the
mandatory screenshot-review pass without any focus juggling.

## The harness traps, encoded so they cannot recur

- **One fresh TCP connection per command** (`dev-harness.md` trap #2 — a held-open
  socket returns stale `shot` frames). `_tcp()` reconnects every call.
- **Never navigate to the screen you're already on** (trap #1 — the router pushes a
  dead duplicate). `app_screen` refuses when the title already matches; `app_list` is
  the duplicate detector.
- **A minimized window stops rendering** (trap #3) — `app_shot`'s doc says so; use
  `mode='offscreen'` for headless shots or `app_foreground()` first.
- **`click` emits the signal; `tap` is a real mouse event** — both exposed, the
  difference documented on the tools (it's a whole class of bug).
- **PyBoy reads battery RAM from `<rom>.gb.ram`** — `drive_session.py` copies the
  (optionally forged) sav there itself; callers never touch it.

## Forging saves — shared, importable

`scripts/emu/forge_save.py` is the STANDING METHOD (any map/position/flags, checksum
resealed) extracted into one importable module + CLI, so probes, the session child and
the MCP server share a single implementation. `emu_forge_save` / `emu_boot` take
map/x/y/flags/`all_flags`/raw `pokes` and reseal automatically.

## Setup & registration

```powershell
pwsh -File scripts/mcp/setup.ps1      # tmp/mcp-venv: mcp + psutil (dev-only, git-ignored)
```

Register `scripts\mcp\run.cmd` as a local (stdio) MCP server in the Claude desktop
config; the launcher bootstraps the venv if missing and keeps stdout clean for the
protocol. Licensing footing is identical to the emu venv
([`emulator-verification.md`](emulator-verification.md) → Licensing): dev-only,
never linked into or shipped with the editor.

✅ **Registered on this machine (2026-07-16):** `mcpServers.pokered-dev` added to the
Claude desktop config (the physical file lives at
`C:\Users\juneh\AppData\Local\Packages\Claude_pzs8sxrjxfjjc\LocalCache\Roaming\Claude\claude_desktop_config.json`
— the `%APPDATA%\Claude` path is the same file through MSIX virtualization; a
`.bak-2026-07-16` sits beside it), pointing at the venv python + `pse_dev_mcp.py`
directly. **Restart the Claude desktop app to load it.** Launch smoke-tested: the
registered command comes up and stays alive on stdio.

## Limits / honesty

- The server assumes this machine's kit paths (they are constants at the top of
  `pse_dev_mcp.py`) — it is a dev tool for this workstation, not a shipped artifact.
- `emu_*` needs the local-only ROM and `tmp/emu-venv`; without them the tools say so
  and everything else still works.
- One emulator session at a time (deliberate — one PyBoy per process, one process at
  a time keeps the machine honest). A new `emu_boot` recycles the child cleanly.
