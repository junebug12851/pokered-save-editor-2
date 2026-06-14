# Player / Rival name edit hang — diagnosis + fix

Status: **FIXED in s13w.** (Root-caused s13v, fixed s13w.) Symptom the maintainer reported: clicking / editing the
**rival** name (and "probably the player name too") makes the app crash/hang. The maintainer suspected "a
circular reference from a hack to get the rival and player name variables working." The analysis below
confirms the player path; rival was light. **What was actually done is in "The fix (s13w)" at the
bottom — read that first if you just want the current state.**

## What actually happens

The **player** name is the heavy one. `PlayerBasics::playerName` is wired:

```cpp
Q_PROPERTY(QString playerName READ getPlayerName WRITE fullSetPlayerName NOTIFY playerNameChanged)
```

`fullSetPlayerName(QString val)` (playerbasics.cpp) does, on **every** write:

1. `getNonTradeMons()` — scans the **whole party + every storage box** (`maxPokemonBoxes`) and every
   Pokémon in each box, calling `hasTradeStatus()` per mon.
2. sets `playerName = val`,
3. `fixNonTradeMons()` — calls `changeOtData(true, this)` on **every** non-trade mon (rewrites OT name
   bytes), then
4. emits `playerNameChanged()` **unconditionally — there is no `if(val == playerName) return;` guard.**

Now the QML two-way bind (`fragments/screens/trainer-card/PlayerNameEdit.qml`):

```qml
onStrChanged: brg.file.data.dataExpanded.player.basics.playerName = str   // → fullSetPlayerName()
Connections { target: ...player.basics
  function onPlayerNameChanged() { top.str = ...playerName } }            // writes back
```

So **each keystroke** in the player name triggers a full save-wide OT scan + rewrite. On a populated
save (hundreds of boxed mons) that's enough work per character to look like a freeze/hang, and it
**writes OT bytes on every keystroke** — which also violates the project's byte-fidelity value
(`context/principles.md` → "Save File Integrity Is Sacred": only write bytes you were told to).

`Rival::name` by contrast is a plain `Q_PROPERTY(... MEMBER name ...)` (cheap, moc-guarded), so the
rival editor itself is light — the maintainer's rival report may be the same UI pattern feeling sluggish, or a
separate issue. The rival two-way bind in `Rival.qml` is the same shape but cheap.

## Secondary issue — missing null guards

`PlayerNameEdit.qml` dereferences `brg.file.data.dataExpanded.player.basics` with **no null guard**,
unlike `Rival.qml` which guards every access with `if(r)`. During load/reset transitions
`dataExpanded` can be null → hard crash. Mirror the rival guards.

## Recommended fix (next pass)

1. **Equality guard in `fullSetPlayerName`** (and `fullSetPlayerId`): `if(val == playerName) return;`
   before doing any work. Cheap, kills redundant scans, and protects byte-fidelity. *(Low risk.)*
2. **Don't full-set per keystroke.** Bind the field one-way *down* (str ← playerName) and only call the
   heavy `fullSetPlayerName` on **commit** (editing finished / accept), not on every `onStrChanged`.
   Or split a cheap `setPlayerName` (just the name) from the OT-cascade, and run the cascade once on
   commit.
3. **Add null guards** to `PlayerNameEdit.qml` matching `Rival.qml`.
4. Re-evaluate the manual two-way signal round-trip; a single committed write removes the loop-y feel
   entirely.

Confirm with a debug-build stack trace if a *hard crash* (not just the hang) still reproduces after 1+3.

## The fix (s13w)

Implemented a cleaner version of the recommendations above. **Needs a rebuild** (C++ changed).

**C++**
- `PlayerBasics::fullSetPlayerName` / `fullSetPlayerId`: added an **equality guard** at the top
  (`if(val == playerName) return;`). No-op when unchanged → no storage rescan, no OT byte writes, no
  signal — and it kills the QML two-way bind's feedback loop.
- `PokemonBox::changeOtData` (the `removeOtData` / adopt-player-OT branch): now **idempotent** — only
  assigns + emits `otNameChanged`/`otIDChanged` for a field that actually differs. Stops the owned-mon
  OT sync from firing a storm of no-op change signals.

**QML — commit on finish, not per keystroke (this is the real cleanup)**
- `NameDisplay` gained a **`committed(string val)`** signal, emitted when an edit session *finishes*:
  the quick-edit popup closing, or the full keyboard closing. A `suppressNextCommit` flag skips the
  popup's commit when it closes only to hand off to the keyboard, so the model is written **exactly
  once** per session.
- `PlayerNameEdit.qml`, `Rival.qml`: persist on **`onCommitted`** (atomic) instead of per-keystroke
  `onStrChanged`, with null guards on the `dataExpanded` chain. `PlayerIdEdit.qml`: same idea via
  `onEditingFinished` (the ID was the identical mess on the `fullSetPlayerId` side); invalid/partial
  input reverts to the stored value.

**Why per-keystroke was not just slow but WRONG:** the OT cascade captures "owned" mons by comparing
their OT to the *current* player name. Typing char-by-char meant an intermediate value (e.g. "AB"
while typing "ABC") could momentarily equal a **traded** mon's OT and sweep it into the owned set,
permanently rewriting that traded mon's OT — a byte-fidelity violation. One atomic commit eliminates
this entirely. (The editors are modal, so the user can't save mid-edit; commit-on-close always lands
before any save.)

**Shared-component note:** `NameDisplay` is shared by player / rival / nickname. The nickname
(`OverviewTab.qml`) deliberately still writes on `onStrChanged` — its `nickname` setter is a cheap
`MEMBER` with no cascade, so per-keystroke is fine there. Only player/rival moved to `committed`.

**UX change to confirm with the maintainer:** the player **ID** now applies on Enter/focus-out instead of live
per digit. If the maintainer wants it live, revert `PlayerIdEdit` to `onTextChanged` — but then it keeps the
(rare) intermediate-collision risk for IDs.
