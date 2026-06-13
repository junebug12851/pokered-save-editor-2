# PC Box Format / Recovery — `pret/pokered` source findings

Source-verified research into how Gen 1 (Red/Blue) handles the "unformatted" box state, to
ground a possible **box-recovery feature** in the editor. All line references are to a local
`pret/pokered` clone at `C:\Users\juneh\Documents\projects\pokered` (cloned 2026-06-13; the
scanning reference for save-format questions — see [[reference_pokered_clone]]).

> **Bottom line:** the in-game "format" of the boxes is **far less destructive than assumed** —
> it only zeroes a 2-byte header per box and leaves every Pokémon record physically intact in
> SRAM. The only true full wipe is the secret clear-save screen. This makes a recovery feature
> genuinely viable.

## The flag (what the app calls `boxesFormatted`)

- `wCurrentBoxNum` (`ram/wram.asm:1901`) holds the current box index in bits 0–6; **bit 7 is
  `BIT_HAS_CHANGED_BOXES`** (`= 7`, `constants/`) — *"whether the player has changed boxes
  before."* The app reads this exact bit at save offset `0x284C` and calls it `boxesFormatted`.
- So "unformatted" really means **"the player has never changed PC boxes."** While this bit is 0,
  the game considers the 12 SRAM boxes uninitialised and ignores them (it only ever has the
  *current* box in WRAM, cached at `sCurBoxData`). The editor mirrors this exactly (it doesn't
  expand `0x4000`/`0x6000` when the bit is off).

## SRAM box layout (`ram/sram.asm`)

- Boxes don't fit one SRAM bank: **`sBox1`–`sBox6` + `sBank2AllBoxesChecksum` + 6 individual
  checksums** in bank "Saved Boxes 1"; **`sBox7`–`sBox12` + `sBank3…` checksums** in "Saved
  Boxes 2". Each box = `wBoxDataEnd - wBoxDataStart`.
- Box struct (`ram/wram.asm:2228`): `wBoxCount` (1) · `wBoxSpecies` (`MONS_PER_BOX+1` = 21, the
  species list + `0xFF` terminator) · `wBoxMons` (20 × `box_struct` = 20 × `0x21`) ·
  `wBoxMonOT` (20 × `NAME_LENGTH`) · `wBoxMonNicks` (20 × `NAME_LENGTH`). `MONS_PER_BOX = 20`,
  `NUM_BOXES = 12`. (The `0x21` record size matches `PokemonBox`'s `recordSize`.)

## The ONLY two routines that write the 12 SRAM boxes

Everything else (normal saves, new game) writes only `sGameData` (`sGameData`→`sGameDataEnd`,
which is the player/main/sprite/party/**current-box-cache**/tile-anim block + main checksum) —
that block is in bank 1 and **excludes** the 12 boxes. So nothing but these two ever touches them:

1. **`ChangeBox`** (PC box switch — `engine/menus/save.asm:358`). On the **first** box change
   (`bit BIT_HAS_CHANGED_BOXES` of `wCurrentBoxNum` is 0) it calls **`EmptyAllSRAMBoxes`**, then
   copies the old box WRAM→SRAM, sets bit 7, sets the new box number, copies the new box
   SRAM→WRAM, and saves. Subsequent switches just swap (bit 7 already set).

2. **`ClearAllSRAMBanks`** (`engine/menus/save.asm:704`) — *"Fill SRAM with $ff, erasing save
   data."* Pads **all four SRAM banks** with `0xFF`. Called **only** by `DoClearSaveDialogue`
   (`engine/movie/oak_speech/clear_save.asm`), the secret "Clear saved data?" screen, and **only
   if the player chooses YES**. Choosing NO `jp Init` — starts the game **without** wiping. This
   is the one true hard wipe.

## `EmptyAllSRAMBoxes` is NOT a real erase (the key finding)

`engine/menus/save.asm:529` → `EmptySRAMBox` (`:568`):

```
EmptySRAMBox:
    xor a        ; a = 0
    ld [hli], a  ; box[0] (count)      = 0x00
    dec a        ; a = 0xFF
    ld [hl], a   ; box[1] (species[0]) = 0xFF   <- now reads as "empty list"
    ret
```

It writes **two bytes per box** — count → `0x00`, first species → `0xFF` (so the species list
reads as immediately terminated = empty) — for all 12 boxes, then recomputes the bank/individual
checksums. **The 20 mon records, OT names, and nicknames below the header are left completely
intact.** The data isn't destroyed; it's *orphaned* (no longer referenced by the count). It only
truly goes away when the player deposits new mons over those record bytes, or runs clear-save.

## New game does NOT wipe boxes (confirmed)

Declining clear-save jumps to `Init`; normal saves only write `sGameData` (boxes excluded). So a
fresh game started over an old cartridge **leaves the old box records sitting in SRAM** until a
box change (2-byte empty) or clear-save (full `0xFF` wipe). Twilight's recollection was correct.

## Implications for an editor recovery feature

Two tiers, by how the data got "lost":

- **Tier 1 — fully intact + checksum-valid.** A save that's unformatted but whose boxes the game
  never emptied (e.g. the editor flipped the flag off and saved — the editor, like the game, does
  NOT write the box banks when the flag is off, so the original full box structures *and* their
  matching checksums survive byte-for-byte). Recoverable with **high confidence**: validate
  structure + verify `sBank{2,3}` checksums, then just expand normally and flip the flag on.

- **Tier 2 — header-emptied, records survive.** Boxes the *game* emptied via `EmptyAllSRAMBoxes`.
  The count + first species byte are gone and the checksum now matches an *empty* box, so the
  checksum is **useless as a gate** here. Recovery is **heuristic**: scan the fixed record slots
  (`wBoxMons`, 20 × `0x21`), validate each independently (species in the real list, level 1–100,
  sane stats/types), recover the first mon's species from its own record byte 0, infer the count
  from the run of valid records. Lower confidence; present as a reviewable preview, opt-in.

**Design stance (unchanged):** keep the main editor strict (ignore unformatted boxes as now); put
all parsing of untrusted box bytes in **one isolated, read-only, fail-closed scanner** behind an
opt-in "scan for recoverable boxes" action (or a separate tool). Never feed unvalidated records
into the live model.

## Note on the in-app warning copy

The Unformat warning currently says the in-game box switch "formats the other boxes and erases
them for good." Functionally true *to the player* (the game shows them gone and offers no way
back), but technically the bytes survive the switch — they're only gone after new deposits or a
clear-save. If we ship recovery, that copy should soften to "the game treats them as gone" rather
than "erases them." Flagged for Twilight; not changed unilaterally.
