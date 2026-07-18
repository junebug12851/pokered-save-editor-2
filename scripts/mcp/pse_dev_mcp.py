#!/usr/bin/env python3
"""pokered-dev — the project's dev-workflow MCP server.

One standardized, reliable transport for the whole rapid-dev loop, replacing the
fragile interactive-shell orchestration that leaked PyBoy processes and hung (see
notes/reference/emulator-verification.md -> "Flag-scenario test framework" and
notes/reference/dev-mcp.md). The rules it encodes:

  * EVERYTHING RUNS IN THE BACKGROUND. Builds, tests, emulator runs are hidden
    console-less child processes logged to tmp/mcp-jobs/. The app launches
    offscreen or minimized by default; app_foreground() is the explicit "bring
    it to the user" moment (per the standing 2026-07-12 rule).
  * NOTHING CAN LEAK. Every long operation is a JOB with a hard timeout whose
    overrun kills the whole process tree (taskkill /T /F); the server kills all
    live jobs and sessions at shutdown; procs_cleanup() sweeps strays.
  * ONE PYBOY PER PROCESS. The emulator session is a child REPL
    (scripts/emu/drive_session.py) that boots exactly once; a new boot means a
    new process. Single-shot probes stay single-shot.
  * THE EXACT TOOLCHAIN. The build env is the Qt Creator kit's, verbatim:
    llvm-mingw + Qt 6.11 + CMake bins prepended to PATH, CC=clang CXX=clang++.
    The kit dir (what the app runs from) and the test build dir are distinct
    and never mixed up. Test/app children get QT_QPA_PLATFORM=offscreen +
    QT_QPA_FONTDIR when headless. Crash-fast error mode is inherited by every
    child, so a crashed exe fails fast instead of hanging on a debugger dialog.

Run it under tmp/mcp-venv (scripts/mcp/setup.ps1 creates it): the launcher is
scripts/mcp/run.cmd. Requires: mcp, psutil (dev-only; never shipped).
"""
from __future__ import annotations

import ctypes
import importlib.util
import json
import os
import queue
import socket
import subprocess
import sys
import threading
import time
import uuid
from pathlib import Path

from mcp.server.fastmcp import FastMCP, Image

# ---------------------------------------------------------------- paths & env

REPO = Path(__file__).resolve().parents[2]
KIT_DIR = REPO / "projects" / "build" / "Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug"
TEST_BUILD = REPO / "build"
APP_EXE = KIT_DIR / "PokeredSaveEditor.exe"
DEFAULT_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
EMU_PY = REPO / "tmp" / "emu-venv" / "Scripts" / "python.exe"
ROM = REPO / "assets" / "references" / "backup.gb"
JOB_DIR = REPO / "tmp" / "mcp-jobs"
SHOT_DIR = REPO / "tmp" / "mcp-shots"

TOOL_BIN = r"C:\Qt\Tools\llvm-mingw1706_64\bin"
QT_BIN = r"C:\Qt\6.11.0\llvm-mingw_64\bin"
CMAKE_BIN = r"C:\Qt\Tools\CMake_64\bin"
FONT_DIR = r"C:\Windows\Fonts"

APP_PORT = 8766
CREATE_NO_WINDOW = 0x08000000
CREATE_NEW_PROCESS_GROUP = 0x00000200
HIDDEN = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP

# Crash-fast: children inherit the error mode, so a crashing test exe dies with an
# exit code instead of hanging the run on the qtcdebugger/WER dialog.
ctypes.windll.kernel32.SetErrorMode(0x0003)  # SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX


def build_env(offscreen: bool = False) -> dict:
    env = os.environ.copy()
    env["PATH"] = ";".join([TOOL_BIN, QT_BIN, CMAKE_BIN]) + ";" + env.get("PATH", "")
    env["CC"] = "clang"
    env["CXX"] = "clang++"
    if offscreen:
        env["QT_QPA_PLATFORM"] = "offscreen"
        env["QT_QPA_FONTDIR"] = FONT_DIR
    return env


def kill_tree(pid: int) -> str:
    r = subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"],
                       capture_output=True, text=True, creationflags=CREATE_NO_WINDOW)
    return (r.stdout + r.stderr).strip()


# ---------------------------------------------------------------- job manager

class Job:
    def __init__(self, desc: str, timeout_s: int):
        self.id = uuid.uuid4().hex[:8]
        self.desc = desc
        self.timeout_s = timeout_s
        self.started = time.time()
        self.status = "running"        # running | done | failed | timeout | killed
        self.exit_code: int | None = None
        JOB_DIR.mkdir(parents=True, exist_ok=True)
        self.log_path = JOB_DIR / f"{self.id}.log"
        self.proc: subprocess.Popen | None = None

    def summary(self) -> dict:
        return {"job_id": self.id, "desc": self.desc, "status": self.status,
                "exit_code": self.exit_code,
                "elapsed_s": round(time.time() - self.started, 1),
                "log": str(self.log_path)}


JOBS: dict[str, Job] = {}
_JOBS_LOCK = threading.Lock()


def spawn_job(desc: str, argv: list[str], cwd: Path, env: dict,
              timeout_s: int) -> dict:
    """Start a hidden, logged, timeout-guarded subprocess job."""
    job = Job(desc, timeout_s)
    log = open(job.log_path, "w", encoding="utf-8", errors="replace")
    log.write(f"$ {' '.join(argv)}\n(cwd {cwd})\n\n")
    log.flush()
    job.proc = subprocess.Popen(argv, cwd=str(cwd), env=env, stdout=log,
                                stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                creationflags=HIDDEN)

    def monitor():
        try:
            job.exit_code = job.proc.wait(timeout=timeout_s)
            job.status = "done" if job.exit_code == 0 else "failed"
        except subprocess.TimeoutExpired:
            kill_tree(job.proc.pid)
            job.status = "timeout"
            job.exit_code = job.proc.wait()
        finally:
            log.close()

    threading.Thread(target=monitor, daemon=True).start()
    with _JOBS_LOCK:
        JOBS[job.id] = job
    return job.summary()


def spawn_fn_job(desc: str, fn, timeout_s: int) -> dict:
    """A job whose body is a Python function `fn(job, log_write)` in a thread.
    The function must launch only timeout-guarded subprocesses itself."""
    job = Job(desc, timeout_s)

    def runner():
        with open(job.log_path, "w", encoding="utf-8", errors="replace") as log:
            def w(text):
                log.write(text)
                log.flush()
            try:
                job.exit_code = fn(job, w)
                job.status = "done" if job.exit_code == 0 else "failed"
            except Exception as e:
                w(f"\nEXCEPTION: {e!r}\n")
                job.status = "failed"
                job.exit_code = -1

    threading.Thread(target=runner, daemon=True).start()
    with _JOBS_LOCK:
        JOBS[job.id] = job
    return job.summary()


def _tail(path: Path, lines: int) -> str:
    if not path.exists():
        return ""
    text = path.read_text(encoding="utf-8", errors="replace")
    return "\n".join(text.splitlines()[-lines:])


# ---------------------------------------------------------------- MCP server

mcp = FastMCP(
    "pokered-dev",
    instructions="Dev-workflow server for pokered-save-editor-2. Long operations "
                 "return a job_id immediately — poll with job_status/job_wait/job_log. "
                 "Everything runs hidden/background by default; use app_foreground "
                 "only when the work is ready for the user to look at.")


# ------------------------------------------------------------------- info

@mcp.tool()
def workspace_info() -> dict:
    """Project + toolchain snapshot: paths, VERSION, git branch, what exists
    (app exe, test build, emu venv, ROM, PyBoy version), and live jobs/sessions."""
    def _git(*args):
        r = subprocess.run(["git", *args], cwd=str(REPO), capture_output=True,
                           text=True, creationflags=CREATE_NO_WINDOW)
        return r.stdout.strip()
    pyboy_ver = ""
    if EMU_PY.exists():
        r = subprocess.run([str(EMU_PY), "-c",
                            "from importlib.metadata import version; print(version('pyboy'))"],
                           capture_output=True, text=True, timeout=30,
                           creationflags=CREATE_NO_WINDOW)
        pyboy_ver = r.stdout.strip()
    return {
        "repo": str(REPO),
        "version": next((ln.strip() for ln in (REPO / "VERSION").read_text().splitlines()
                         if ln.strip() and not ln.strip().startswith("#")), "?")
                   if (REPO / "VERSION").exists() else "?",
        "git_branch": _git("rev-parse", "--abbrev-ref", "HEAD"),
        "git_head": _git("log", "--oneline", "-1"),
        "kit_dir": str(KIT_DIR), "app_exe_present": APP_EXE.exists(),
        "test_build_dir": str(TEST_BUILD), "test_build_configured": (TEST_BUILD / "CMakeCache.txt").exists(),
        "emu_venv_present": EMU_PY.exists(), "pyboy_version": pyboy_ver,
        "rom_present": ROM.exists(),
        "jobs_running": [j.summary() for j in JOBS.values() if j.status == "running"],
        "emu_session_live": _SESSION["proc"] is not None,
        "app_running": _app_pid() is not None,
    }


# ------------------------------------------------------------------- build & test

@mcp.tool()
def build_app(clean: bool = False, timeout_s: int = 1800) -> dict:
    """Build the APP (kit dir Makefiles build — the exe that actually runs:
    projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug). This is the build
    for in-app testing; note editing a savefile .cpp rebuilds the DLL, not the exe
    (verify by DLL timestamp). Returns a job — poll with job_wait/job_status."""
    argv = [CMAKE_BIN + r"\cmake.exe", "--build", str(KIT_DIR),
            "--target", "PokeredSaveEditor", "--parallel"]
    if clean:
        argv.insert(2, "--clean-first")
    return spawn_job("build app (kit dir)", argv, REPO, build_env(), timeout_s)


@mcp.tool()
def build_tests(target: str = "", clean: bool = False, timeout_s: int = 2400) -> dict:
    """Build the TEST build dir (repo-root build/, Ninja). target='' builds all;
    or name one (e.g. 'tst_warps', 'tst_flag_scenarios'). Returns a job."""
    argv = [CMAKE_BIN + r"\cmake.exe", "--build", str(TEST_BUILD), "--parallel"]
    if target:
        argv += ["--target", target]
    if clean:
        argv.insert(2, "--clean-first")
    return spawn_job(f"build tests ({target or 'all'})", argv, REPO, build_env(), timeout_s)


@mcp.tool()
def configure_tests(fresh: bool = False, timeout_s: int = 900) -> dict:
    """(Re)configure the test build dir: cmake -S projects -B build -G Ninja with the
    kit toolchain env. fresh=True adds --fresh (wipe the cache). Returns a job."""
    argv = [CMAKE_BIN + r"\cmake.exe", "-S", "projects", "-B", "build", "-G", "Ninja"]
    if fresh:
        argv.append("--fresh")
    return spawn_job("configure tests", argv, REPO, build_env(), timeout_s)


@mcp.tool()
def run_ctest(filter_regex: str = "", timeout_s: int = 3600) -> dict:
    """Run ctest in the test build dir (offscreen, crash-fast). filter_regex maps to
    -R; empty = the FULL suite. Returns a job — read the verdict with job_log."""
    argv = [CMAKE_BIN + r"\ctest.exe", "--output-on-failure"]
    if filter_regex:
        argv += ["-R", filter_regex]
    return spawn_job(f"ctest {filter_regex or '(full)'}", argv, TEST_BUILD,
                     build_env(offscreen=True), timeout_s)


@mcp.tool()
def run_test_exe(name: str, args: list[str] | None = None, timeout_s: int = 900) -> dict:
    """Run one test executable from build/ directly (e.g. name='tst_qml_screens'),
    offscreen with fonts + Qt DLLs on PATH. Returns a job."""
    exe = TEST_BUILD / (name if name.endswith(".exe") else name + ".exe")
    if not exe.exists():
        return {"error": f"{exe} not found — build_tests(target='{name}') first"}
    return spawn_job(f"run {name}", [str(exe), *(args or [])], TEST_BUILD,
                     build_env(offscreen=True), timeout_s)


@mcp.tool()
def capture_screenshots(timeout_s: int = 1200) -> dict:
    """Run the headless screenshooter batch (scripts/capture_screenshots.ps1) —
    renders every screen to tmp/screenshots/ as still PNGs, never writes a save.
    Returns a job."""
    argv = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
            "-File", str(REPO / "scripts" / "capture_screenshots.ps1")]
    return spawn_job("capture screenshots", argv, REPO, build_env(), timeout_s)


# ------------------------------------------------------------------- jobs

@mcp.tool()
def job_status(job_id: str, tail_lines: int = 40) -> dict:
    """Status of a job + the tail of its log."""
    job = JOBS.get(job_id)
    if not job:
        return {"error": f"no job {job_id}"}
    return {**job.summary(), "log_tail": _tail(job.log_path, tail_lines)}


@mcp.tool()
def job_wait(job_id: str, timeout_s: int = 120, tail_lines: int = 60) -> dict:
    """Block (bounded) until the job finishes; returns final status + log tail.
    Re-call if it is still running after timeout_s."""
    job = JOBS.get(job_id)
    if not job:
        return {"error": f"no job {job_id}"}
    deadline = time.time() + min(timeout_s, 570)
    while job.status == "running" and time.time() < deadline:
        time.sleep(0.5)
    return {**job.summary(), "log_tail": _tail(job.log_path, tail_lines)}


@mcp.tool()
def job_log(job_id: str, tail_lines: int = 300) -> str:
    """The tail of a job's full log."""
    job = JOBS.get(job_id)
    if not job:
        return f"no job {job_id}"
    return _tail(job.log_path, tail_lines)


@mcp.tool()
def job_kill(job_id: str) -> dict:
    """Kill a running job's whole process tree."""
    job = JOBS.get(job_id)
    if not job:
        return {"error": f"no job {job_id}"}
    if job.proc and job.status == "running":
        kill_tree(job.proc.pid)
        job.status = "killed"
    return job.summary()


@mcp.tool()
def job_list() -> list[dict]:
    """All jobs this server session (newest last)."""
    return [j.summary() for j in JOBS.values()]


# ------------------------------------------------------------------- the app

_APP: dict = {"proc": None, "mode": ""}


def _app_pid() -> int | None:
    p = _APP.get("proc")
    if p is not None and p.poll() is None:
        return p.pid
    try:
        with socket.create_connection(("127.0.0.1", APP_PORT), timeout=0.6):
            return -1     # running, but not launched by us
    except OSError:
        return None


def _tcp(cmd: dict, timeout: float = 15.0) -> dict:
    """One fresh connection per command — REQUIRED (a held-open socket returns
    stale frames for `shot`; see reference/dev-harness.md trap #2)."""
    try:
        with socket.create_connection(("127.0.0.1", APP_PORT), timeout=timeout) as s:
            s.sendall((json.dumps(cmd) + "\n").encode("utf-8"))
            f = s.makefile("r", encoding="utf-8")
            line = f.readline()
        return json.loads(line) if line.strip() else {"error": "empty reply"}
    except OSError as e:
        return {"error": f"app control channel unreachable ({e}) — is the app "
                         f"running (app_launch) and a DEBUG build?"}


def _find_hwnds(pid: int) -> list[int]:
    user32 = ctypes.windll.user32
    hwnds: list[int] = []
    proto = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)

    def cb(hwnd, _):
        wpid = ctypes.c_ulong()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(wpid))
        if wpid.value == pid and user32.IsWindowVisible(hwnd):
            hwnds.append(hwnd)
        return True

    user32.EnumWindows(proto(cb), 0)
    return hwnds


@mcp.tool()
def app_launch(sav: str = "", screen: str = "", select: str = "", hot: bool = True,
               mode: str = "background", extra_args: list[str] | None = None) -> dict:
    """Launch the editor via the DEBUG harness. mode:
      'offscreen'  — headless (QT_QPA_PLATFORM=offscreen): no window at all; shots
                     work; best for pure automation. (Pixel-art shader falls back.)
      'background' — real window, started MINIMIZED, steals no focus (default).
                     NOTE: a minimized window stops rendering, so app_shot returns
                     stale frames until app_foreground() restores it.
      'foreground' — normal window, for the user's live pass.
    sav defaults to the BaseSAV fixture; screen ∈ registered names (trainerCard,
    pokedex, bag, pokemart, pokemon, rival, maps, home, pokemonDetails, ...);
    select e.g. 'party:0'. Uses the app's own --sav/--screen/--select/--hot/
    --minimized flags. Waits for the 127.0.0.1:8766 control channel."""
    if _app_pid() is not None:
        return {"error": "app already running — app_stop() first (or drive it as-is)"}
    if not APP_EXE.exists():
        return {"error": f"{APP_EXE} missing — build_app() first"}
    argv = [str(APP_EXE)]
    sav_path = Path(sav) if sav else DEFAULT_SAV
    if not sav_path.is_absolute():
        sav_path = REPO / sav_path
    argv += ["--sav", str(sav_path)]
    if screen:
        argv += ["--screen", screen]
    if select:
        argv += ["--select", select]
    if hot:
        argv.append("--hot")
    if mode == "background":
        argv.append("--minimized")
    argv += (extra_args or [])
    env = build_env(offscreen=(mode == "offscreen"))
    _APP["proc"] = subprocess.Popen(argv, cwd=str(REPO), env=env,
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                    creationflags=CREATE_NEW_PROCESS_GROUP)
    _APP["mode"] = mode
    ok = False
    for _ in range(60):                      # up to ~30 s for boot + DB load
        time.sleep(0.5)
        if _tcp({"cmd": "ping"}, timeout=1.0).get("error") is None:
            ok = True
            break
        if _APP["proc"].poll() is not None:
            return {"error": f"app exited immediately (code {_APP['proc'].poll()})"}
    return {"pid": _APP["proc"].pid, "mode": mode, "control_channel": ok,
            "argv": argv}


@mcp.tool()
def app_stop() -> dict:
    """Stop the running editor (kills the process tree if needed)."""
    p = _APP.get("proc")
    if p is not None and p.poll() is None:
        kill_tree(p.pid)
        _APP["proc"] = None
        return {"stopped": True}
    if _app_pid() == -1:
        return {"error": "an editor is running but was not launched by this server — "
                         "close it manually or via procs_cleanup(include_app=True)"}
    return {"stopped": False, "note": "not running"}


@mcp.tool()
def app_foreground() -> dict:
    """Bring the editor window to the FRONT for the user to look at — the explicit
    'it's ready for her' moment. Restores from minimized and activates."""
    p = _APP.get("proc")
    if p is None or p.poll() is not None:
        return {"error": "no app launched by this server"}
    user32 = ctypes.windll.user32
    hwnds = _find_hwnds(p.pid)
    if not hwnds:
        return {"error": "no visible window (offscreen mode has none — relaunch "
                         "with mode='foreground' or 'background')"}
    for h in hwnds:
        user32.ShowWindow(h, 9)              # SW_RESTORE
        user32.SwitchToThisWindow(h, True)
    return {"foregrounded": len(hwnds)}


@mcp.tool()
def app_background() -> dict:
    """Minimize the editor window out of the way (no focus steal thereafter)."""
    p = _APP.get("proc")
    if p is None or p.poll() is not None:
        return {"error": "no app launched by this server"}
    user32 = ctypes.windll.user32
    hwnds = _find_hwnds(p.pid)
    for h in hwnds:
        user32.ShowWindow(h, 6)              # SW_MINIMIZE
    return {"minimized": len(hwnds)}


@mcp.tool()
def app_cmd(command_json: str, timeout_s: int = 15) -> dict:
    """RAW passthrough to the app's TCP control channel (one JSON object).
    Verbs: ping, screen, party, back, home, sav, shot, title, get, set, click,
    tap, invoke, reload, list. See notes/reference/dev-harness.md."""
    try:
        cmd = json.loads(command_json)
    except json.JSONDecodeError as e:
        return {"error": f"bad JSON: {e}"}
    return _tcp(cmd, timeout=timeout_s)


@mcp.tool()
def app_screen(name: str) -> dict:
    """Navigate to a screen (waits for the transition to finish before replying).
    Navigating to the screen you're already on is refused by the harness itself
    (reply carries "already": true) — the duplicate-push trap cannot recur."""
    return _tcp({"cmd": "screen", "arg": name}, timeout=20)


@mcp.tool()
def app_title() -> dict:
    """Current screen: human title (result) + registered screen name (screen)."""
    return _tcp({"cmd": "title"})


@mcp.tool()
def app_load_sav(path: str) -> dict:
    """Load a .sav into the running app."""
    p = Path(path)
    if not p.is_absolute():
        p = REPO / p
    return _tcp({"cmd": "sav", "arg": str(p).replace("\\", "/")})


@mcp.tool()
def app_get(obj: str, prop: str) -> dict:
    """Read a QML property. obj = objectName, or a '/'-separated path from a named
    node ('mapRightPanel/0/2'); numbers index direct children in visual order."""
    return _tcp({"cmd": "get", "obj": obj, "prop": prop})


@mcp.tool()
def app_set(obj: str, prop: str, value: str) -> dict:
    """Write a QML property (value auto-converts)."""
    return _tcp({"cmd": "set", "obj": obj, "prop": prop, "val": value})


@mcp.tool()
def app_click(obj: str) -> dict:
    """Emit a control's clicked() SIGNAL directly. ⚠️ This exercises none of the
    event-delivery machinery — for 'what happens when the user actually clicks
    there' use app_tap instead."""
    return _tcp({"cmd": "click", "obj": obj})


@mcp.tool()
def app_tap(obj: str = "", x: int = -1, y: int = -1, button: str = "left",
            double: bool = False) -> dict:
    """A REAL mouse press+release through Qt's delivery path (grabs, handlers,
    propagation). Target by objectName/path OR by view coordinates."""
    cmd: dict = {"cmd": "tap"}
    if obj:
        cmd["obj"] = obj
    else:
        if x < 0 or y < 0:
            return {"error": "give obj or x+y"}
        cmd["x"], cmd["y"] = x, y
    if button != "left":
        cmd["button"] = button
    if double:
        cmd["double"] = True
    return _tcp(cmd)


@mcp.tool()
def app_hover(obj: str = "", x: int = -1, y: int = -1) -> dict:
    """Move the POINTER (no button) to an item's centre or a view coordinate --
    a genuine MouseMove through Qt's delivery. Drives everything hover does:
    containsMouse, HoverHandler, the map's cell cursor, tab strips appearing,
    tooltips (hover, wait out the delay, then app_shot to capture one)."""
    cmd: dict = {"cmd": "hover"}
    if obj:
        cmd["obj"] = obj
    else:
        if x < 0 or y < 0:
            return {"error": "give obj or x+y"}
        cmd["x"], cmd["y"] = x, y
    return _tcp(cmd)


@mcp.tool()
def app_drag(obj: str = "", x: int = -1, y: int = -1,
             tobj: str = "", tx: int = -1, ty: int = -1, steps: int = 12) -> dict:
    """A REAL drag: press at the source (objectName/path or x+y), interpolated
    moves with the button held, release at the target (tobj or tx+ty). Drives
    sprite/warp/sign dragging and the tab proxy-drag exactly as a person would."""
    cmd: dict = {"cmd": "drag", "steps": steps}
    if obj:
        cmd["obj"] = obj
    elif x >= 0 and y >= 0:
        cmd["x"], cmd["y"] = x, y
    else:
        return {"error": "give obj or x+y for the source"}
    if tobj:
        cmd["tobj"] = tobj
    elif tx >= 0 and ty >= 0:
        cmd["tx"], cmd["ty"] = tx, ty
    else:
        return {"error": "give tobj or tx+ty for the target"}
    return _tcp(cmd)


@mcp.tool()
def app_scroll(obj: str, to: str = "", y: int = -1) -> dict:
    """Scroll a panel to a NAMED position: to='top'/'start', 'bottom'/'end', or
    any objectName inside the content (lands it near the viewport top). Or a raw
    contentY via y. `obj` may be the ScrollView, the Flickable, or any container
    -- the first scrollable descendant is used."""
    cmd: dict = {"cmd": "scroll", "obj": obj}
    if y >= 0:
        cmd["y"] = y
    elif to:
        cmd["to"] = to
    else:
        return {"error": "give to= (top/bottom/objectName) or y="}
    return _tcp(cmd)


@mcp.tool()
def app_invoke(obj: str, method: str, args: list[str] | None = None) -> dict:
    """Emit any signal / call any slot or Q_INVOKABLE by name (no-arg is the
    always-works case; args pass as QVariant)."""
    cmd = {"cmd": "invoke", "obj": obj, "method": method}
    if args:
        cmd["args"] = args
    return _tcp(cmd)


@mcp.tool()
def app_list(query: str = "", obj: str = "") -> dict:
    """List reachable objectNames matching substring `query`, or (obj='name')
    that node's direct children as '[index] TypeName' for path navigation.
    Also THE duplicate-screen detector: the same name three times means dead
    copies are stacked."""
    cmd: dict = {"cmd": "list"}
    if obj:
        cmd["obj"] = obj
    elif query:
        cmd["arg"] = query
    return _tcp(cmd)


@mcp.tool()
def app_reload() -> dict:
    """Force a full QML hot-reload (stays on the current screen, save survives)."""
    return _tcp({"cmd": "reload"}, timeout=30)


@mcp.tool()
def app_flow(steps: list[dict], stop_on_error: bool = True) -> dict:
    """MULTI-STEP app driving in ONE call (complex screens — the map screen —
    without per-step round-trips). Each step is one op:
      {"op":"screen","name":"maps"}          navigate (refuses duplicates)
      {"op":"get","obj":o,"prop":p}          read a property
      {"op":"set","obj":o,"prop":p,"val":v}  write a property
      {"op":"tap","obj":o} / {"op":"tap","x":..,"y":..}   real mouse event
      {"op":"click","obj":o}                 emit clicked() (signal only)
      {"op":"invoke","obj":o,"method":m,"args":[..]}
      {"op":"wait","obj":o,"prop":p,"equals":v,"timeout_s":5}   poll until true
      {"op":"assert","obj":o,"prop":p,"equals":v}               fail if not
      {"op":"sleep","ms":300}
      {"op":"sav","path":"..."}              load a save
      {"op":"shot","label":"x","obj":?}      screenshot -> file path in results
      {"op":"list","query":?,"obj":?} · {"op":"back"} · {"op":"home"} ·
      {"op":"reload"} · {"op":"raw","cmd":{...}}
    Per-step results return in order; stop_on_error halts at the first failure.
    Needs the app running (app_launch) — DEBUG harness on 127.0.0.1:8766."""
    results = []
    ok_all = True
    for i, s in enumerate(steps):
        op = s.get("op", "")
        r: dict
        if op == "screen":
            r = _tcp({"cmd": "screen", "arg": s.get("name", "")}, timeout=20)
        elif op == "get":
            r = _tcp({"cmd": "get", "obj": s.get("obj", ""), "prop": s.get("prop", "")})
        elif op == "set":
            r = _tcp({"cmd": "set", "obj": s.get("obj", ""), "prop": s.get("prop", ""),
                      "val": s.get("val", "")})
        elif op == "tap":
            cmd: dict = {"cmd": "tap"}
            if s.get("obj"):
                cmd["obj"] = s["obj"]
            else:
                cmd["x"], cmd["y"] = s.get("x", -1), s.get("y", -1)
            if s.get("button", "left") != "left":
                cmd["button"] = s["button"]
            if s.get("double"):
                cmd["double"] = True
            r = _tcp(cmd)
        elif op == "click":
            r = _tcp({"cmd": "click", "obj": s.get("obj", "")})
        elif op == "invoke":
            cmd = {"cmd": "invoke", "obj": s.get("obj", ""), "method": s.get("method", "")}
            if s.get("args"):
                cmd["args"] = s["args"]
            r = _tcp(cmd)
        elif op in ("wait", "assert"):
            want = str(s.get("equals", ""))
            deadline = time.time() + (float(s.get("timeout_s", 5)) if op == "wait" else 0)
            while True:
                g = _tcp({"cmd": "get", "obj": s.get("obj", ""), "prop": s.get("prop", "")})
                got = str(g.get("result", g.get("error", "")))
                if got == want:
                    r = {"result": got}
                    break
                if time.time() >= deadline:
                    r = {"error": f"{op} failed: {s.get('obj')}.{s.get('prop')} "
                                  f"= {got!r}, wanted {want!r}"}
                    break
                time.sleep(0.25)
        elif op == "sleep":
            time.sleep(min(int(s.get("ms", 250)), 30000) / 1000.0)
            r = {"result": "slept"}
        elif op == "sav":
            p = Path(s.get("path", ""))
            if not p.is_absolute():
                p = REPO / p
            r = _tcp({"cmd": "sav", "arg": str(p).replace("\\", "/")})
        elif op == "shot":
            SHOT_DIR.mkdir(parents=True, exist_ok=True)
            p = SHOT_DIR / f"{s.get('label', 'flow')}-{time.strftime('%H%M%S')}-{i}.png"
            cmd = {"cmd": "shot", "arg": str(p).replace("\\", "/"),
                   "settle": s.get("settle_ms", 250)}
            if s.get("obj"):
                cmd["obj"] = s["obj"]
            r = _tcp(cmd, timeout=30)
            if not r.get("error"):
                r["path"] = str(p)
        elif op == "list":
            cmd = {"cmd": "list"}
            if s.get("obj"):
                cmd["obj"] = s["obj"]
            elif s.get("query"):
                cmd["arg"] = s["query"]
            r = _tcp(cmd)
        elif op in ("back", "home", "reload"):
            r = _tcp({"cmd": op}, timeout=30)
        elif op == "raw":
            r = _tcp(s.get("cmd") or {}, timeout=s.get("timeout_s", 15))
        else:
            r = {"error": f"unknown op {op!r}"}
        results.append({"step": i, "op": op, **r})
        if r.get("error") is not None:
            ok_all = False
            if stop_on_error:
                break
    return {"ok": ok_all, "results": results}


@mcp.tool()
def app_shot(obj: str = "", label: str = "shot", settle_ms: int = 250) -> Image:
    """Screenshot the app view (or just one component by objectName/path) via the
    harness's framebuffer grab and RETURN THE IMAGE inline. Saved under
    tmp/mcp-shots/ too. The harness waits out any in-flight screen transition and
    then settle_ms more (animations/fades), so mid-transition "distorted" grabs
    can't happen. Works while backgrounded/offscreen — but NOT while minimized
    (stale frames): app_foreground() or offscreen mode first."""
    SHOT_DIR.mkdir(parents=True, exist_ok=True)
    p = SHOT_DIR / f"{label}-{time.strftime('%H%M%S')}.png"
    cmd: dict = {"cmd": "shot", "arg": str(p).replace("\\", "/"),
                 "settle": settle_ms}
    if obj:
        cmd["obj"] = obj
    r = _tcp(cmd, timeout=30)
    if r.get("error") or not p.exists():
        raise RuntimeError(f"shot failed: {r}")
    return Image(path=str(p))


# ------------------------------------------------------------------- emulator

_SESSION: dict = {"proc": None, "q": None}
_SESSION_LOCK = threading.Lock()


def _session_kill(reason: str = "") -> None:
    p = _SESSION.get("proc")
    if p is not None and p.poll() is None:
        kill_tree(p.pid)
    _SESSION["proc"] = None
    _SESSION["q"] = None


def _session_send(obj: dict, timeout_s: float) -> dict:
    with _SESSION_LOCK:
        p = _SESSION.get("proc")
        if p is None or p.poll() is not None:
            return {"ok": False, "error": "no live emulator session — emu_boot first"}
        try:
            p.stdin.write(json.dumps(obj) + "\n")
            p.stdin.flush()
        except OSError as e:
            _session_kill()
            return {"ok": False, "error": f"session pipe broke ({e}); session killed"}
        try:
            return _SESSION["q"].get(timeout=timeout_s)
        except queue.Empty:
            _session_kill()
            return {"ok": False, "error": f"session did not answer within {timeout_s}s — "
                                          f"killed the whole tree (nothing leaks)"}


def _emu_available() -> str:
    if not ROM.exists():
        return "no ROM at assets/references/backup.gb (local-only)"
    if not EMU_PY.exists():
        return "no emulator venv — run emu_setup()"
    return ""


@mcp.tool()
def emu_status() -> dict:
    """Emulator environment health: venv, PyBoy version, ROM presence + SHA-1
    match, live session, and any stray emu-venv python processes."""
    out: dict = {"venv_present": EMU_PY.exists(), "rom_present": ROM.exists(),
                 "session_live": _SESSION["proc"] is not None and _SESSION["proc"].poll() is None}
    if EMU_PY.exists():
        r = subprocess.run([str(EMU_PY), "-c",
                            "from importlib.metadata import version; print(version('pyboy'))"],
                           capture_output=True, text=True, timeout=60,
                           creationflags=CREATE_NO_WINDOW)
        out["pyboy_version"] = r.stdout.strip() or r.stderr.strip()
    if ROM.exists():
        import hashlib
        out["rom_sha1"] = hashlib.sha1(ROM.read_bytes()).hexdigest()
        out["rom_sha1_expected"] = "ea9bcae617fdf159b045185467ae58b2e4a48b9a"
        out["rom_ok"] = out["rom_sha1"] == out["rom_sha1_expected"]
    out["stray_processes"] = _stray_procs()
    return out


@mcp.tool()
def emu_setup(recreate: bool = False, timeout_s: int = 900) -> dict:
    """Create (or -Recreate from scratch) the tmp/emu-venv PyBoy venv via
    scripts/emu/setup.ps1. Returns a job."""
    argv = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
            "-File", str(REPO / "scripts" / "emu" / "setup.ps1")]
    if recreate:
        argv.append("-Recreate")
    return spawn_job("emu venv setup", argv, REPO, os.environ.copy(), timeout_s)


@mcp.tool()
def emu_check_updates(timeout_s: int = 240) -> dict:
    """Check whether PyBoy (or anything else in the emu venv) has an update
    available (pip list --outdated). Returns a job (network-bound)."""
    if not EMU_PY.exists():
        return {"error": "no emu venv — emu_setup() first"}
    argv = [str(EMU_PY), "-m", "pip", "list", "--outdated", "--format", "json"]
    return spawn_job("emu pip outdated", argv, REPO, os.environ.copy(), timeout_s)


@mcp.tool()
def emu_update(package: str = "pyboy", timeout_s: int = 600) -> dict:
    """Upgrade a package in the emu venv (default PyBoy). Returns a job.
    Run the parity suite afterwards — a new PyBoy must still satisfy the oracle."""
    if not EMU_PY.exists():
        return {"error": "no emu venv — emu_setup() first"}
    argv = [str(EMU_PY), "-m", "pip", "install", "--upgrade", package]
    return spawn_job(f"emu pip upgrade {package}", argv, REPO, os.environ.copy(), timeout_s)


@mcp.tool()
def emu_run_script(script: str, args: list[str] | None = None,
                   timeout_s: int = 300) -> dict:
    """Run one scripts/emu/*.py probe SINGLE-SHOT under the emu venv (the sanctioned
    shape: boots once, writes results, exits; killed as a tree on overrun).
    script may be a bare name ('probe_warp_persistence.py'). Returns a job."""
    why = _emu_available()
    if why:
        return {"error": why}
    sp = Path(script)
    if not sp.is_absolute():
        sp = REPO / "scripts" / "emu" / sp.name if not script.startswith(("scripts", "tmp")) \
            else REPO / script
    if not sp.exists():
        return {"error": f"{sp} not found"}
    argv = [str(EMU_PY), str(sp), *(args or [])]
    return spawn_job(f"emu {sp.name}", argv, REPO, os.environ.copy(), timeout_s)


@mcp.tool()
def emu_flag_scenarios(only: str = "", per_scenario_timeout_s: int = 180) -> dict:
    """Run the event-flag scenario batch (scripts/emu/flag_scenarios.json) the safe
    way: ONE SCENARIO PER PROCESS (PyBoy wedges if re-instantiated in-process),
    each child timeout-guarded and reaped. only='name' runs one. Returns a job;
    results also land in tmp/emu/scn_<name>.json."""
    why = _emu_available()
    if why:
        return {"error": why}
    scenarios = json.loads((REPO / "scripts" / "emu" / "flag_scenarios.json")
                           .read_text(encoding="utf-8"))
    names = [s["name"] for s in scenarios if not only or s["name"] == only]
    if not names:
        return {"error": f"no scenario named '{only}'"}

    def body(job: Job, w) -> int:
        rc_all = 0
        for name in names:
            out = REPO / "tmp" / "emu" / f"scn_{name}.json"
            argv = [str(EMU_PY), str(REPO / "scripts" / "emu" / "run_flag_scenarios.py"),
                    "--only", name, "--out", str(out)]
            w(f"== {name}\n")
            pr = subprocess.Popen(argv, cwd=str(REPO), stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True,
                                  creationflags=HIDDEN)
            try:
                sout, _ = pr.communicate(timeout=per_scenario_timeout_s)
                w(sout or "")
                if pr.returncode != 0:
                    rc_all = pr.returncode
            except subprocess.TimeoutExpired:
                # A wedge is a VERDICT: a hard-crashed console executes STOP, the
                # frame never completes, PyBoy's tick() spins forever. Kill the
                # whole tree (venv python is a launcher; a plain kill leaks the
                # interpreter) and record "hang" as the scenario's result.
                kill_tree(pr.pid)
                pr.wait()
                w(f"HANG after {per_scenario_timeout_s}s — console wedged; tree killed\n")
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_text(json.dumps({"results": [
                    {"name": name, "result": "hang"}]}, indent=1))
        return rc_all

    return spawn_fn_job(f"flag scenarios ({only or 'all ' + str(len(names))})",
                        body, per_scenario_timeout_s * (len(names) + 1))


MAP_SAVES = REPO / "tmp" / "emu" / "map-saves"


def _forge_module():
    spec = importlib.util.spec_from_file_location(
        "forge_save", REPO / "scripts" / "emu" / "forge_save.py")
    fs = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fs)
    return fs


def _ensure_map_base(map_id: int, warp: int = 0, force: bool = False,
                     timeout_s: int = 420) -> Path:
    """The console-authored base save for a map (tmp/emu/map-saves/mapNNN.sav) —
    generated by forge_map_save.py in a fresh, tree-kill-guarded subprocess if
    missing. The game itself walks there through a hijacked door and the settled
    WRAM is dumped, so every pointer is engine-authored."""
    cache = MAP_SAVES / f"map{map_id:03d}.sav"
    if cache.exists() and not force:
        return cache
    argv = [str(EMU_PY), str(REPO / "scripts" / "emu" / "forge_map_save.py"),
            "--map", str(map_id), "--warp", str(warp)]
    if force:
        argv.append("--force")
    pr = subprocess.Popen(argv, cwd=str(REPO), stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True, creationflags=HIDDEN)
    try:
        sout, _ = pr.communicate(timeout=timeout_s)
    except subprocess.TimeoutExpired:
        kill_tree(pr.pid)
        pr.wait()
        raise RuntimeError(f"map-save generation timed out for {map_id:#04x} "
                           f"(tree killed — nothing leaks)")
    if pr.returncode != 0 or not cache.exists():
        raise RuntimeError(f"map-save generation failed for {map_id:#04x}:\n{sout}")
    return cache


@mcp.tool()
def emu_make_map_save(map_id: int, warp: int = 0, force: bool = False,
                      timeout_s: int = 420) -> dict:
    """Generate (or fetch from cache) a CONSOLE-AUTHORED save at any map: the real
    game is booted and walked there through a hijacked door (an edge-hop via a
    neighbour for the 8 warpless routes), and the settled WRAM is dumped in
    SaveSAVtoSRAM's own layout — every pointer, sprite slot, connection and wild
    table authored by the engine itself, self-checked, checksum resealed. `warp`
    picks the arrival warp on the target. Cached at tmp/emu/map-saves/."""
    try:
        cache = _ensure_map_base(map_id, warp, force, timeout_s)
    except RuntimeError as e:
        return {"error": str(e)}
    side = MAP_SAVES / f"map{map_id:03d}.json"
    return {"sav": str(cache),
            **(json.loads(side.read_text()) if side.exists() else {})}


@mcp.tool()
def emu_forge_save(out_path: str, map_id: int = -1, x: int = -1, y: int = -1,
                   flags: list[str] | None = None, flag_indices: list[int] | None = None,
                   all_flags: bool = False, pokes: dict[str, str] | None = None,
                   scripts: dict | None = None, filter_flags: dict | None = None,
                   base_sav: str = "") -> dict:
    """Forge a save at ANY map / position / flag state (checksum resealed).
    map_id >= 0 uses the CONSOLE-AUTHORED base for that map (generated on the fly
    if not cached — the game walks there itself; see emu_make_map_save), so the
    whole Area block is genuinely that map's. Coordinates go through relocate():
    block coords + view pointer stay in sync.
      flags        = pret EVENT_* names to set
      scripts      = per-map script step, e.g. {"Route 22": 0}  (w<Map>CurScript)
      filter_flags = missable visibility, e.g. {"Route 22/Rival 1": "show"}  (or a
                     bit index -> "show"/"hide")
      pokes        = raw byte writes {'0x29B9': '0x01'} for anything else
    Together these give TOTAL trigger control — enough to arm and reproduce a
    scripted cutscene (see probe_route22_conflict.py)."""
    fs = _forge_module()
    if map_id >= 0:
        base = _ensure_map_base(map_id)
    else:
        base = Path(base_sav) if base_sav else DEFAULT_SAV
        if not base.is_absolute():
            base = REPO / base
    out = Path(out_path)
    if not out.is_absolute():
        out = REPO / out
    data = fs.forge(base.read_bytes(),
                    None,                                   # base already IS the map
                    None if y < 0 else y, None if x < 0 else x,
                    flags or None, flag_indices or None, all_flags,
                    {int(k, 0): int(v, 0) for k, v in (pokes or {}).items()},
                    scripts=scripts, filter_flags=filter_flags)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)
    return {"path": str(out), "bytes": len(data),
            "map": data[0x260A], "x": data[0x260E], "y": data[0x260D]}


@mcp.tool()
def emu_boot(sav: str = "", map_id: int = -1, x: int = -1, y: int = -1,
             flags: list[str] | None = None, all_flags: bool = False,
             pokes: dict[str, str] | None = None,
             scripts: dict | None = None, filter_flags: dict | None = None,
             mash: bool = True,
             budget_frames: int = 4000, timeout_s: int = 150) -> dict:
    """Start an INTERACTIVE emulator session at ANY custom state: map_id >= 0
    boots the CONSOLE-AUTHORED save for that map (generated on the fly if not
    cached — allow ~60s extra the first time; see emu_make_map_save), with
    optional x/y relocation (derived bytes kept in sync), EVENT_* flags, per-map
    script steps (scripts={"Route 22": 0}), missable visibility
    (filter_flags={"Route 22/Rival 1": "show"}), and raw pokes — the
    total-custom-state resume, enough to ARM a scripted cutscene. The child is a
    fresh owned process (scripts/emu/drive_session.py); drive it with
    emu_button/emu_tick/emu_mem/emu_poke/emu_screenshot. One session at a time;
    a re-boot = a fresh process."""
    why = _emu_available()
    if why:
        return {"error": why}
    if map_id >= 0:
        if sav:
            return {"error": "give sav OR map_id, not both"}
        try:
            sav = str(_ensure_map_base(map_id))
        except RuntimeError as e:
            return {"error": str(e)}
    with _SESSION_LOCK:
        _session_kill()
        proc = subprocess.Popen([str(EMU_PY), str(REPO / "scripts" / "emu" / "drive_session.py")],
                                cwd=str(REPO), stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, text=True, encoding="utf-8",
                                creationflags=HIDDEN)
        q: queue.Queue = queue.Queue()

        def reader():
            for line in proc.stdout:
                line = line.strip()
                if line.startswith("{"):
                    try:
                        q.put(json.loads(line))
                    except json.JSONDecodeError:
                        pass
            q.put({"ok": False, "error": "session process ended"})

        threading.Thread(target=reader, daemon=True).start()
        _SESSION["proc"] = proc
        _SESSION["q"] = q
        try:
            ready = q.get(timeout=60)
        except queue.Empty:
            _session_kill()
            return {"error": "session child never came up (60s)"}
        if not ready.get("ready"):
            _session_kill()
            return {"error": f"session child unavailable: {ready}"}
    boot_cmd: dict = {"cmd": "boot", "budget": budget_frames, "mash": mash}
    if sav:
        p = Path(sav)
        boot_cmd["sav"] = str(p if p.is_absolute() else REPO / p)
    # (never send a raw "map" — a map-id-only forge is a chimera; the map_id path
    # above already swapped in the console-authored base save for that map)
    if x >= 0:
        boot_cmd["x"] = x
    if y >= 0:
        boot_cmd["y"] = y
    if flags:
        boot_cmd["flags"] = flags
    if all_flags:
        boot_cmd["all_flags"] = True
    if pokes:
        boot_cmd["pokes"] = pokes
    if scripts:
        boot_cmd["scripts"] = scripts          # {"Route 22": 0} — per-map script step
    if filter_flags:
        boot_cmd["filter_flags"] = filter_flags  # {"Route 22/Rival 1": "show"}
    return _session_send(boot_cmd, timeout_s)


@mcp.tool()
def emu_button(button: str, hold_frames: int = 4, after_frames: int = 30,
               timeout_s: int = 60) -> dict:
    """Press a button in the live session (a/b/start/select/up/down/left/right),
    hold it hold_frames, then run after_frames. Returns the game state."""
    return _session_send({"cmd": "button", "b": button, "hold": hold_frames,
                          "after": after_frames}, timeout_s)


@mcp.tool()
def emu_tick(frames: int = 60, timeout_s: int = 120) -> dict:
    """Advance the live session N frames (59.7275 Hz). Returns the game state."""
    return _session_send({"cmd": "tick", "n": frames}, timeout_s)


@mcp.tool()
def emu_mem(addr: str, length: int = 1, timeout_s: int = 30) -> dict:
    """Read emulator memory (hex string back). addr hex ok: '0xD35E'. Read ANY
    console state — WRAM, the sound engine at 0xC000, OAM, whatever."""
    return _session_send({"cmd": "mem", "addr": addr, "len": length}, timeout_s)


@mcp.tool()
def emu_poke(addr: str, hex_bytes: str, timeout_s: int = 30) -> dict:
    """Write bytes into emulator memory (live), e.g. addr='0xD35E', hex_bytes='21'."""
    return _session_send({"cmd": "poke", "addr": addr, "bytes": hex_bytes}, timeout_s)


@mcp.tool()
def emu_state(timeout_s: int = 30) -> dict:
    """Current game state of the live session (map/dims/coords/battle/on_overworld)."""
    return _session_send({"cmd": "state"}, timeout_s)


@mcp.tool()
def emu_screenshot(label: str = "emu", timeout_s: int = 30) -> Image:
    """Screenshot the live session's 160x144 framebuffer and RETURN THE IMAGE
    (also saved under tmp/mcp-shots/)."""
    SHOT_DIR.mkdir(parents=True, exist_ok=True)
    p = SHOT_DIR / f"{label}-{time.strftime('%H%M%S')}.png"
    r = _session_send({"cmd": "shot", "path": str(p)}, timeout_s)
    if not r.get("ok") or not p.exists():
        raise RuntimeError(f"emu shot failed: {r}")
    return Image(path=str(p))


@mcp.tool()
def emu_stop() -> dict:
    """End the interactive emulator session (graceful quit, then tree-kill)."""
    r = _session_send({"cmd": "quit"}, 10)
    _session_kill()
    return {"stopped": True, "last_reply": r}


# ---------------------------------------------------------------- autopilot
# Pathfinding + auto-navigation executed INSIDE the session child (one process
# does the hundreds of steps; this layer plans, forwards, reports). Plans come
# from our own shipped map data; every step is verified against the console's
# WRAM. Design: notes/plans/dev-autopilot.md.

_NAV_WORLD = {"w": None}


def _nav_world():
    if _NAV_WORLD["w"] is None:
        sys.path.insert(0, str(REPO / "scripts" / "emu"))
        import navigate
        _NAV_WORLD["w"] = navigate.World()
    return _NAV_WORLD["w"]


def _resolve_map(ref) -> dict:
    e = _nav_world().resolve(ref)
    return e


def _session_live() -> bool:
    p = _SESSION.get("proc")
    return p is not None and p.poll() is None


@mcp.tool()
def emu_walk_to(x: int, y: int, on_battle: str = "run", on_trainer: str = "stop",
                max_steps: int = 800, timeout_s: int = 420) -> dict:
    """Walk the player to (x, y) ON THE CURRENT MAP — A* over our own collision
    data (ledge hops included), every step verified against WRAM, live NPCs
    treated as moving collision (bounded retries, then re-plan around them).
    Wild battles en route follow on_battle ('run'|'mash'|'stop'); trainer
    battles follow on_trainer ('stop'|'mash'). Needs a live emu_boot session."""
    return _session_send({"cmd": "walk_to", "x": x, "y": y,
                          "on_battle": on_battle, "on_trainer": on_trainer,
                          "max_steps": max_steps}, timeout_s)


@mcp.tool()
def emu_goto(map: str, x: int = -1, y: int = -1, on_battle: str = "run",
             on_trainer: str = "stop", start_map: str = "",
             approach: str = "natural", flags: list[str] | None = None,
             unblock: bool = True, bike: bool = True,
             surf: str = "auto", cut: str = "auto",
             timeout_s: int = 1200) -> dict:
    """THE one-call 'take me there': navigate to a map (name, modernName, or id —
    'Mt Moon B1F', 'Mt. Moon 2', '60', '0x3C'), optionally to exact (x, y),
    CROSS-MAP (warps, connections, doors, ladders, ELEVATORS — planned, then
    walked for real with per-step WRAM verification).

    No live session? One is booted automatically. approach='natural' (default)
    boots ONE MAP OUT — a map that warps/connects into the target — and WALKS IN
    for real, so wLastMap and the whole entry state are authored by the walk
    (dropping in at the Pokécenter doorstep, arriving in the cave from the route
    outside). approach='direct' boots the target map's base straight away.
    start_map='...' overrides both (a real journey from there).
    `flags` = EVENT_* names for the boot forge.

    Progression aids (all reported in `prep`, all opt-out): unblock=True sets
    the Saffron guard-drink bit when a gate is on the route; bike=True puts a
    BICYCLE in the bag when Cycling Road is; surf/cut='auto' plan dry first and
    open water / cuttable trees only when no dry route exists ('always'/'never'
    to force). With a live session it navigates from wherever the game stands."""
    try:
        dst = _resolve_map(map)
    except KeyError as e:
        return {"error": str(e)}
    booted_at = None
    if not _session_live():
        w = _nav_world()
        candidates: list[int] = []
        if start_map:
            try:
                candidates = [_resolve_map(start_map)["ind"]]
            except KeyError as e:
                return {"error": str(e)}
        elif approach == "natural":
            # one map out, cached bases first (a miss costs a ~60s generation)
            apps = w.approaches_of(dst["ind"])
            cached = [a for a in apps
                      if (MAP_SAVES / f"map{a:03d}.sav").exists()]
            candidates = (cached + [a for a in apps if a not in cached])[:3] \
                + [dst["ind"]]
        else:
            candidates = [dst["ind"]]
        r = None
        for boot_map in candidates:
            r = emu_boot(map_id=boot_map, flags=flags)
            if not r.get("error") and r.get("ok", True):
                booted_at = boot_map
                break
        if booted_at is None:
            return {"error": f"auto-boot failed everywhere: {r}"}
    out = _session_send({"cmd": "goto", "map": dst["ind"], "x": x, "y": y,
                         "on_battle": on_battle, "on_trainer": on_trainer,
                         "unblock": unblock, "bike": bike,
                         "surf": surf, "cut": cut}, timeout_s)
    if booted_at is not None and isinstance(out, dict):
        out["booted_at"] = {"map": booted_at,
                            "name": _nav_world().by_ind[booted_at]["name"],
                            "approach": "natural" if booted_at != dst["ind"]
                            else "direct"}
    return out


@mcp.tool()
def emu_set_flag(flag: str, on: bool = True, timeout_s: int = 30) -> dict:
    """Set/clear one event flag in the LIVE session's WRAM — by pret EVENT_*
    name or raw bit index. The story lever: arm/disarm world state mid-run
    (reported, never silent)."""
    return _session_send({"cmd": "set_flag", "flag": flag, "on": on}, timeout_s)


@mcp.tool()
def emu_give_item(item: int, qty: int = 1, timeout_s: int = 30) -> dict:
    """Put an item in the LIVE bag (pret item id, e.g. 6 = BICYCLE). Bumps the
    quantity when already carried; honors the 20-slot cap."""
    return _session_send({"cmd": "give_item", "item": item, "qty": qty},
                         timeout_s)


@mcp.tool()
def emu_move_sprite(slot: int, x: int, y: int, timeout_s: int = 30) -> dict:
    """Relocate an NPC/boulder sprite in LIVE WRAM (slot 1..15). The Strength-
    boulder lever — put the rock on the switch — and the unblock-a-doorway
    lever. Reported, never silent."""
    return _session_send({"cmd": "move_sprite", "slot": slot, "x": x, "y": y},
                         timeout_s)


@mcp.tool()
def emu_talk_to(target: str, dismiss: bool = True, timeout_s: int = 300) -> dict:
    """Talk to an NPC on the current map — a MOVING target: its live square is
    chased (re-planned as it wanders), the player stands adjacent, faces it,
    presses A, and the text box is confirmed open. target = sprite slot (1..15)
    or the maps.json sprite name ('Bug Catcher'). dismiss=True reads the text to
    the end (a trainer's line starting a battle is reported, never hidden)."""
    return _session_send({"cmd": "talk", "target": target, "dismiss": dismiss},
                         timeout_s)


@mcp.tool()
def emu_battle(policy: str = "mash", timeout_s: int = 600) -> dict:
    """Execute the CURRENT battle a certain way: 'mash' (A to the end),
    'run' (flee a wild battle; trainer battles refuse with the reason),
    'move:N' (use move slot N every turn; B declines level-up move prompts),
    'sweep' (WIN on request — enemy HP held at 1 in WRAM, every mon of a
    trainer's team falls to the next hit; the poke is reported). 'sweep' +
    emu_talk_to(leader) is the win-a-gym-battle recipe."""
    return _session_send({"cmd": "battle", "policy": policy}, timeout_s)


@mcp.tool()
def emu_hunt_encounter(max_steps: int = 240, policy: str = "stop",
                       timeout_s: int = 600) -> dict:
    """FIND a wild battle here: pace a two-square shuttle (grass squares
    preferred — the map's own wGrassTile, read live; caves encounter anywhere)
    until wIsInBattle fires. Reports the enemy species/level. policy 'stop'
    hands the live battle back for emu_battle; 'run'/'mash'/'move:N' resolve it."""
    return _session_send({"cmd": "hunt", "max_steps": max_steps,
                          "policy": policy}, timeout_s)


@mcp.tool()
def emu_dismiss(timeout_s: int = 60) -> dict:
    """Clear any open text boxes (B-heavy mash — never buys/selects/learns)."""
    return _session_send({"cmd": "dismiss"}, timeout_s)


@mcp.tool()
def emu_train(level: int, slot: int = 1, policy: str = "sweep",
              max_battles: int = 60, timeout_s: int = 1800) -> dict:
    """TRAIN a party mon to a level: hunt + win on repeat (policy 'sweep' by
    default — certain wins, real XP; 'mash'/'move:N' for legit fights).
    Evolution prompts are declined (the mon stays itself). Stand somewhere
    with encounters (grass/cave) — emu_goto there first."""
    return _session_send({"cmd": "train", "level": level, "slot": slot,
                          "policy": policy, "max_battles": max_battles},
                         timeout_s)


@mcp.tool()
def emu_save_game(timeout_s: int = 120) -> dict:
    """Save IN-GAME the player's way: start menu → SAVE → YES, waited to
    completion. (PyBoy flushes the battery file at session stop.)"""
    return _session_send({"cmd": "save_game"}, timeout_s)


@mcp.tool()
def emu_heal(timeout_s: int = 180) -> dict:
    """Heal at the Pokémon Center you're standing in: talks to the nurse over
    the counter, verified by every party mon reading HP == max."""
    return _session_send({"cmd": "heal"}, timeout_s)


@mcp.tool()
def emu_mart_buy(item: int, qty: int = 1, timeout_s: int = 240) -> dict:
    """⚠️ EXPERIMENTAL — buy from the mart you're standing in: clerk → BUY →
    the item out of the LIVE shop list → quantity → confirm. The result is
    HONEST (verified by the bag gaining exactly qty + money moving; ok:false
    with the real deltas otherwise), but the multi-stage menu timing is not
    yet deterministic — check the reply, and prefer emu_give_item when the
    item just needs to EXIST. item = pret item id (4 = Poké Ball)."""
    return _session_send({"cmd": "buy", "item": item, "qty": qty}, timeout_s)


@mcp.tool()
def emu_pc_box(action: str = "deposit", slot: int = 1,
               timeout_s: int = 240) -> dict:
    """⚠️ EXPERIMENTAL — Bill's PC in the Center you're standing in:
    'deposit' (party slot → box) or 'withdraw' (box → party). The result is
    HONEST (verified by BOTH counts moving; ok:false with the real counts
    otherwise), but the PC's text→menu cadence is not yet deterministic —
    check the reply and retry, or poke party/box state directly for setup."""
    return _session_send({"cmd": "pc_box", "action": action, "slot": slot},
                         timeout_s)


@mcp.tool()
def emu_party_swap(a: int, b: int, timeout_s: int = 120) -> dict:
    """Reorder the party through the real POKéMON menu (mon a ⇄ mon b,
    1-based) — verified by the species order changing (honest ok:false with
    before/after otherwise; an immediate repeat swap can need a retry)."""
    return _session_send({"cmd": "party_swap", "a": a, "b": b}, timeout_s)


@mcp.tool()
def emu_set_options(text_fast: bool = True, anim_off: bool = True,
                    style_set: bool = True, timeout_s: int = 120) -> dict:
    """Set the in-game OPTIONS through the real menu (text speed / battle
    animation / battle style) — verified against the wOptions byte."""
    return _session_send({"cmd": "set_options", "text_fast": text_fast,
                          "anim_off": anim_off, "style_set": style_set},
                         timeout_s)


@mcp.tool()
def emu_start_select(item: str, timeout_s: int = 60) -> dict:
    """Open the start menu and select an entry by name (POKEDEX/POKEMON/ITEM/
    TRAINER/SAVE/OPTION/EXIT — layout adapts to game progress). The generic
    door into any menu flow; follow with emu_button/emu_play steps."""
    return _session_send({"cmd": "start_select", "item": item}, timeout_s)


@mcp.tool()
def emu_new_game(timeout_s: int = 600) -> dict:
    """Start a FRESH GAME from nothing: boots with no battery save and mashes
    through the title, Oak's intro and both naming screens (names end up
    arbitrary — irrelevant for testing) to the bedroom overworld. From there
    the world is a blank slate (no party — hunt/battle need the starter;
    emu_set_flag/emu_give_item can fast-forward story state)."""
    why = _emu_available()
    if why:
        return {"error": why}
    with _SESSION_LOCK:
        _session_kill()
        proc = subprocess.Popen([str(EMU_PY), str(REPO / "scripts" / "emu" / "drive_session.py")],
                                cwd=str(REPO), stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, text=True, encoding="utf-8",
                                creationflags=HIDDEN)
        q: queue.Queue = queue.Queue()

        def reader():
            for line in proc.stdout:
                line = line.strip()
                if line.startswith("{"):
                    try:
                        q.put(json.loads(line))
                    except json.JSONDecodeError:
                        pass
            q.put({"ok": False, "error": "session process ended"})

        threading.Thread(target=reader, daemon=True).start()
        _SESSION["proc"] = proc
        _SESSION["q"] = q
        try:
            ready = q.get(timeout=60)
        except queue.Empty:
            _session_kill()
            return {"error": "session child never came up (60s)"}
        if not ready.get("ready"):
            _session_kill()
            return {"error": f"session child unavailable: {ready}"}
    return _session_send({"cmd": "boot", "new_game": True}, timeout_s)


@mcp.tool()
def emu_play(steps: list[dict], stop_on_error: bool = True,
             timeout_s: int = 1800) -> dict:
    """A WHOLE RUN in one call — the session child executes the step list
    server-side (no per-step transport round-trips). Each step is any session
    command: {"cmd":"goto","map":"Mt Moon 1F"} · {"cmd":"walk_to","x":5,"y":5} ·
    {"cmd":"hunt","policy":"run"} · {"cmd":"battle","policy":"move:1"} ·
    {"cmd":"talk","target":"Youngster"} · {"cmd":"button","b":"a"} ·
    {"cmd":"tick","n":60} · {"cmd":"poke",...} · {"cmd":"mem",...} ·
    {"cmd":"shot","path":"tmp/mcp-shots/x.png"} · {"cmd":"dismiss"}.
    Short bursts or long hauls; per-step results come back; stop_on_error halts
    at the first failure. (boot/quit are not allowed inside a script.)"""
    return _session_send({"cmd": "script", "steps": steps,
                          "stop_on_error": stop_on_error}, timeout_s)


# ------------------------------------------------------------------- hygiene

def _stray_procs() -> list[dict]:
    import psutil
    marker = str(REPO).lower()
    me = os.getpid()
    out = []
    live = {j.proc.pid for j in JOBS.values() if j.proc and j.status == "running"}
    sess = _SESSION.get("proc")
    if sess and sess.poll() is None:
        live.add(sess.pid)
    for p in psutil.process_iter(["pid", "name", "cmdline", "create_time"]):
        try:
            if p.info["name"] not in ("python.exe", "pythonw.exe"):
                continue
            cl = " ".join(p.info["cmdline"] or []).lower()
            if "emu-venv" not in cl and marker not in cl:
                continue
            if p.info["pid"] in live or p.info["pid"] == me or "mcp-venv" in cl:
                continue
            out.append({"pid": p.info["pid"],
                        "age_s": round(time.time() - p.info["create_time"]),
                        "cmdline": " ".join(p.info["cmdline"] or [])[:160]})
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue
    return out


@mcp.tool()
def procs_cleanup(dry_run: bool = False, include_app: bool = False) -> dict:
    """Sweep LEAKED processes: emu-venv/repo python.exe strays not owned by a live
    job or the session (the exact leak class that starved the machine on
    2026-07-15). include_app=True also kills any running PokeredSaveEditor.
    dry_run lists without killing. ⚠️ An emu test running OUTSIDE this server
    (ctest tst_flag_scenarios / tst_emu_parity) looks like a stray to this sweep —
    don't kill while one is running (dry_run first)."""
    strays = _stray_procs()
    killed = []
    if not dry_run:
        for s in strays:
            kill_tree(s["pid"])
            killed.append(s["pid"])
    apps = []
    if include_app:
        import psutil
        for p in psutil.process_iter(["pid", "name"]):
            if p.info["name"] == "PokeredSaveEditor.exe":
                apps.append(p.info["pid"])
                if not dry_run:
                    kill_tree(p.info["pid"])
        if not dry_run:
            _APP["proc"] = None
    return {"strays": strays, "killed": killed, "app_pids": apps, "dry_run": dry_run}


# ------------------------------------------------------------------- shutdown

def _shutdown():
    _session_kill()
    for j in list(JOBS.values()):
        if j.proc and j.status == "running":
            kill_tree(j.proc.pid)
            j.status = "killed"
    p = _APP.get("proc")
    if p is not None and p.poll() is None and _APP.get("mode") == "offscreen":
        kill_tree(p.pid)     # headless app is invisible — never leave it behind;
                             # a visible window is left for the user to see/close.


import atexit  # noqa: E402
atexit.register(_shutdown)


if __name__ == "__main__":
    mcp.run()
