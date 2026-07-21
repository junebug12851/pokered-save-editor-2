/*
  * Copyright 2026 Fairy Fox
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
#pragma once

#include "./audio_autoport.h"

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

#include <cstdint>
#include <vector>

/**
 * @brief **The game's own sheet music, read as text.**
 *
 * We ship `pret/pokered`'s **`.asm`** — their files, their macros, their words — and turn it into
 * the bytes the sound engine plays. Not a blob we generated somewhere else and asked you to trust.
 *
 * ```asm
 * Music_PalletTown_Ch1:
 *     tempo 144
 *     volume 7, 7
 *     duty_cycle 2
 *     octave 3
 *     note C_, 4
 * ```
 *
 * ⚠️ **THIS IS A LINE PARSER, NOT AN ASSEMBLER.** project leadership's insight, and it is the whole reason this
 * is a tractable amount of code: the sound engine's data is *line-based assembly*, and **their macro
 * names ARE the command names**. `note C_, 8` is one line and one byte. There is no expression
 * evaluator, no arithmetic, no sections, no linker script. There is a table of macros, a scope for
 * local labels, and a list of pointers to fill in at the end.
 *
 * ## ⚠️ Why we still produce BYTES, and not a list of "commands"
 *
 * Because **two thirds of the music would vanish if we didn't.**
 *
 * A track's id is not stored anywhere. It is *computed from its header's address*:
 *
 * ```
 * id = (header_address − SFX_Headers_N) / 3     ; "Song ids are calculated by address to save
 *                                               ;  space" -- constants/music_constants.asm
 * ```
 *
 * A header is 3 bytes **per channel**, so a 3-channel song eats three consecutive ids — and the spare
 * ones are that song's **inner voices**, read as one-channel headers in their own right. That is
 * where 105 of our 151 tracks come from (id 187 is Pallet Town's bassline, playing alone). They exist
 * **because the header table is a contiguous region of bytes and a misaligned id lands mid-header.**
 *
 * A structured command list has nothing to misalign. So the bytes are not an implementation detail we
 * could hide behind a nicer type — **the byte layout IS the feature**, and the addresses have to come
 * out where the cartridge puts them. See notes/reference/glitch-music.md.
 *
 * ## How it is proved
 *
 * `tst_music_asm` demands this parser's output be **byte-identical** to the images
 * `scripts/import_music.py` produces — and that script, run with `--check` against a real cartridge,
 * is byte-diffed against the console itself. So the chain is: **cartridge → Python importer →
 * fixture → this parser.** Nothing here is taken on trust.
 *
 * @see notes/plans/music.md (phase 8), notes/reference/gen1-sound-engine.md
 */
class AUDIO_AUTOPORT Gen1MusicAsm
{
public:
  /// One audio bank, assembled: the 256-entry header table at `$4000`, then every stream it reaches.
  struct Bank {
    std::vector<uint8_t> data;   ///< Starts at `$4000`. Exactly what Gen1SoundEngine::Bank wants.
    int maxSfxId = 0;            ///< `MAX_SFX_ID_N` — above this, PlaySound takes the music path.
  };

  /// Everything the engine needs, out of the `.asm`.
  struct Result {
    Bank bank2;
    Bank bank8;
    Bank bank31;

    /// The channel-3 wave instruments: 5 real ones, then one `.wave5` **per bank**. @see waves()
    std::vector<uint8_t> waves;

    bool ok = false;
    QString error;    ///< Why not, if `!ok`. Never a silent failure.
  };

  /// The addresses the ROM's audio region starts at. Our images keep it, so the engine's 16-bit
  /// pointers mean the same thing they mean on the cartridge.
  static constexpr int Base = 0x4000;

  /**
   * @brief Read the vendored `.asm` under @p root and assemble all three banks.
   *
   * @p root is a directory holding `headers/`, `music/`, `sfx/` and `wave_samples.asm` — i.e. a copy
   * of `pret/pokered/audio`. In the app that is a Qt resource path; in the tests it is a directory.
   *
   * ⚠️ It reads **every** `.asm` in `music/` and `sfx/`, not just the ones a track needs. It has to:
   * the SFX headers come *first* in the id table, so an SFX we failed to place would shift every
   * music id after it — and a shifted id is a different song.
   */
  static Result parse(const QString& root);

private:
  /// One source file, assembled into a relocatable blob with named labels and pending pointers.
  struct Stream {
    QString file;
    QByteArray data;
    QHash<QString, int> labels;                  ///< label -> offset within `data`
    QVector<QPair<int, QString>> fixups;         ///< (offset of the 2-byte pointer, target label)
  };

  static bool assemble(const QString& path, const QString& text, Stream& out, QString& err);
  static bool op(Stream& s, const QString& op, const QStringList& args, int chan, QString& err);
};
