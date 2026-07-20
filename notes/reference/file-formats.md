# File formats: **use the game's**

> **Project leadership, 2026-07-13 (a standing rule):**
> *"Perhaps we should try to match pokered file formats where possible like blk files are raw data just
> like pokered and music can be assembly the same assembly straight from the project we just wont compile
> it. We can leverage their macro names and stuff to our advantage because it's much easier to create a
> parser to sound engine commands. Any other file formats they use we should try to use here by default."*

---

## The rule

**Where `pret/pokered` has a format, we use that format.** By default, without asking, without a meeting.

Not "a format inspired by theirs". Not "our own JSON of the same data". **Theirs** — their bytes, their
text, their macro names, their directory shape.

Three reasons, and the third is the one that matters:

1. **It is already correct.** It is the format the game's own source is written in. There is nothing to
   get subtly wrong in a converter, because there is no converter.
2. **It interoperates.** Anyone else's `.blk` drops straight into our editor and ours drops straight into
   theirs. A format only we can read is a format only we can read.
3. **Their macros do half our work.** The music data is not an opaque blob to be reverse-engineered — it
   is *line-based assembly*, with names: `note C_, 8`, `octave 4`, `dutycycle 2`. **A line parser turns
   it straight into sound-engine commands.** We never needed a byte compiler. We just needed to read.

---

## Where we stand

| Data | Format | Status |
|---|---|---|
| **Map blocks** | `.blk` — raw block bytes, exactly as `pret/pokered` stores them | ✅ **done.** Byte-identical. Imported by `scripts/import_map_blocks.ps1`, which re-reads the cartridge and demands a match. |
| **Sprites** | one loose `.png` per sprite | ✅ **done** (2026-07-13). Not an atlas: you can open one and look at it. |
| **Music** | ⚠️ **the game's own `.asm` sheet music, parsed LINE BY LINE** | **owed.** We currently import a compiled byte blob. That was the wrong call: the source is `Music_PalletTown_Ch1: ... note C_, 8`, and a line parser reads it directly. Their macro names become our command names. |
| Tile traits / collision | derived from the ROM's own lists | ✅ done (`scripts/import_tile_traits.py`, verified against the cartridge). |
| **Anything new** | **whatever `pret/pokered` uses** | The default. |

---

## When you are about to design a format

**Stop. Go and look at what they do first.**

Deviating from their format is a *decision*. It gets argued for, and it gets written down in
[`../decisions/architecture.md`](../decisions/architecture.md) with the reason. It is not a thing that
happens because nobody looked.

(The atlas is the cautionary tale: 72 sprites got packed into one sheet nobody could open, because it
seemed tidy, and because nobody asked. It is 72 loose files now.)
