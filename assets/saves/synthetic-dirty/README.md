# Dirty / malformed save fixtures

**These are intentionally broken saves for negative testing — not real saves.** They
exist to prove the editor degrades gracefully (the Sims-2 philosophy: *never crash,
never block, never strand the user, never mutate the source on a failed load*) and to
lock that behaviour in with tests.

They are generated, synthetic, and safe to regenerate. None of them is a real game
save. The structural/garbage files below contain no real save data; checksum-corruption
cases are derived from `../natural-clean/BaseSAV.sav` **at test time** (into a temp dir, not committed)
so this folder never duplicates real save data.

Consumed by `projects/tests/savefile/tst_dirty_fixtures.cpp`.

| File | What's wrong | Intent / expected behaviour |
|------|--------------|------------------------------|
| `empty.sav` | 0 bytes | A picked file that is empty. Load must be **rejected** (a save is exactly 32 KB / `0x8000`), surface an error, and leave any already-open save byte-for-byte untouched. |
| `truncated_4kb.sav` | 4 KB of zeros (way short) | A clearly truncated/garbage file. **Rejected** as too short; no crash; source untouched. |
| `truncated_32767.sav` | `0x7FFF` bytes — exactly one byte short of 32 KB | The off-by-one boundary: the loader requires **at least** `0x8000` bytes. **Rejected**; proves the size check is `< 0x8000`, not `<=`. |
| `oversized_48kb.sav` | 48 KB | Larger than a save. The loader accepts it and reads the **first 32 KB** (documented behaviour); must not crash or read past the buffer. |
| `all_zeros.sav` | Exactly 32 KB, every byte `0x00` | Structurally the right size but semantically blank/garbage. Must **load + expand + flatten without crashing** (empty party/boxes/dex, zeroed fields). A graceful-degradation probe. |
| `all_ff.sav` | Exactly 32 KB, every byte `0xFF` | Right size, every field at its maximum/garbage value (e.g. counts = 255, indices out of range). The harshest expand-path probe — must not crash on out-of-range species/items/counts. |
| `garbage.sav` | Exactly 32 KB of deterministic pseudo-random bytes (seed 1337) | Right size, totally random contents. Must load/expand/flatten without crashing; a fuzz-style smoke that complements `tst_fuzz`. |

## Checksum-corruption cases (derived at test time, not committed)

Built in `tst_dirty_fixtures.cpp` by copying `BaseSAV.sav` to a temp file and flipping
one byte, so no real save data is committed here:

- **Bad main-data checksum** — flip the byte at `0x3523`. The editor is not the game; it
  should still **load** the file (it recomputes checksums on save), not crash or refuse.
- **Bad bank-2 box checksum** — flip the byte at `0x5A4C`. Same: load, don't crash.

## Regenerating the committed files

PowerShell (sizes in bytes; `0x8000` = 32768):

```powershell
$dir = "assets/saves/synthetic-dirty"; $SZ = 0x8000
[IO.File]::WriteAllBytes("$dir/empty.sav",           (New-Object byte[] 0))
[IO.File]::WriteAllBytes("$dir/truncated_4kb.sav",   (New-Object byte[] 4096))
[IO.File]::WriteAllBytes("$dir/truncated_32767.sav", (New-Object byte[] ($SZ-1)))
[IO.File]::WriteAllBytes("$dir/oversized_48kb.sav",  (New-Object byte[] 49152))
[IO.File]::WriteAllBytes("$dir/all_zeros.sav",       (New-Object byte[] $SZ))
$ff = New-Object byte[] $SZ; 0..($SZ-1) | % { $ff[$_] = 0xFF }
[IO.File]::WriteAllBytes("$dir/all_ff.sav", $ff)
$g = New-Object byte[] $SZ; ([Random]::new(1337)).NextBytes($g)
[IO.File]::WriteAllBytes("$dir/garbage.sav", $g)
```
