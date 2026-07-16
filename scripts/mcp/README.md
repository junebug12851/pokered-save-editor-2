# pokered-dev — the dev-workflow MCP server

One standardized, reliable way for an AI session (Claude/Cowork) to drive the whole
rapid-development loop: build, test, launch and drive the app, take screenshots, and
run the Game Boy emulator — with **nothing able to hang, leak, or steal focus**.

Full reference: [`notes/reference/dev-mcp.md`](../../notes/reference/dev-mcp.md).
Why it exists: the interactive shell's ~44 s per-call cap leaked PyBoy processes and
wedged the machine (2026-07-15, see `notes/reference/emulator-verification.md`).

## Setup (once)

```powershell
pwsh -File scripts/mcp/setup.ps1     # creates tmp/mcp-venv, installs mcp + psutil
```

Register in the Claude desktop config (Settings → Extensions/Connectors → local MCP),
command:

```json
{ "pokered-dev": { "command": "C:\\Users\\juneh\\Documents\\QtProjects\\pokered-save-editor-2\\scripts\\mcp\\run.cmd" } }
```

`run.cmd` bootstraps the venv itself if missing. Dev-only, git-ignored venv,
never shipped (same footing as `tmp/emu-venv`).

## The rules it encodes

- **Background by default.** Builds/tests/emu runs are hidden child processes logged
  to `tmp/mcp-jobs/`. The app launches offscreen or minimized; `app_foreground()` is
  the explicit "bring it to the user" moment.
- **Jobs, never blocking.** Long operations return a `job_id` instantly; poll with
  `job_wait`/`job_status`/`job_log`; overruns kill the whole process tree.
- **One PyBoy per process** (it wedges on re-instantiation). The interactive session
  is an owned child REPL (`scripts/emu/drive_session.py`); single-shot probes stay
  single-shot; `emu_flag_scenarios` runs one scenario per process.
- **The exact kit toolchain**: llvm-mingw + Qt 6.11 + CMake on PATH, `CC=clang
  CXX=clang++`, kit dir vs test build dir kept apart, offscreen + fontdir when
  headless, crash-fast error mode inherited by every child.

## Tools

| Area | Tools |
|---|---|
| Info | `workspace_info` |
| Build/test | `build_app` · `build_tests` · `configure_tests` · `run_ctest` · `run_test_exe` · `capture_screenshots` |
| Jobs | `job_status` · `job_wait` · `job_log` · `job_kill` · `job_list` |
| App | `app_launch` · `app_stop` · `app_foreground` · `app_background` · `app_status→workspace_info` · `app_cmd` (raw) · `app_screen` · `app_title` · `app_load_sav` · `app_get/set` · `app_click` · `app_tap` · `app_invoke` · `app_list` · `app_reload` · `app_shot` (image back inline) |
| Emulator | `emu_status` · `emu_setup` · `emu_check_updates` · `emu_update` · `emu_run_script` · `emu_flag_scenarios` · `emu_forge_save` · `emu_boot` · `emu_button` · `emu_tick` · `emu_mem` · `emu_poke` · `emu_state` · `emu_screenshot` (image back inline) · `emu_stop` |
| Hygiene | `procs_cleanup` |

The app tools speak the DEBUG harness's TCP channel (`127.0.0.1:8766`,
`notes/reference/dev-harness.md`) — one fresh connection per command, duplicate-screen
trap guarded. The emu tools require the local-only ROM (`assets/references/backup.gb`)
and `tmp/emu-venv` (`scripts/emu/setup.ps1`); without them they report unavailability
cleanly — nothing here is required to build or ship the editor.
