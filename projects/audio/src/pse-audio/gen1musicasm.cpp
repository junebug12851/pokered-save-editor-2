/*
  * Copyright 2026 Twilight
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
#include "./gen1musicasm.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

#include <functional>

namespace {

/// The twelve semitones, in the game's own order (`audio/notes.asm` / the `note` macro).
const QStringList NoteNames = {
  QStringLiteral("C_"), QStringLiteral("C#"), QStringLiteral("D_"), QStringLiteral("D#"),
  QStringLiteral("E_"), QStringLiteral("F_"), QStringLiteral("F#"), QStringLiteral("G_"),
  QStringLiteral("G#"), QStringLiteral("A_"), QStringLiteral("A#"), QStringLiteral("B_"),
};

int noteIndex(const QString& n) { return NoteNames.indexOf(n); }

/// ⚠️ `.wave5` IS A BUG, AND WE REPRODUCE IT.
///
/// The label has no data behind it, so the game reads whatever bytes happen to follow the wave table
/// **in that bank** — which is why it is different in each of the three. `pokered` records exactly
/// what those bytes are (`audio/wave_samples.asm`), and **Lavender Town and Pokémon Tower are built
/// on the result.** It is not "broken"; it is the instrument. Copied verbatim. Do not "fix" it.
const QVector<int> Wave5Bank2 = {
  2, 1, 14, 2, 3, 3, 2, 8, 14, 1, 2, 2, 15, 15, 14, 10,
  1, 0, 1, 4, 13, 12, 1, 0, 14, 3, 4, 1, 5, 1, 7, 3 };
const QVector<int> Wave5Bank8 = {
  14, 12, 0, 2, 2, 0, 9, 1, 0, 7, 12, 0, 2, 0, 8, 1,
  0, 7, 13, 0, 2, 0, 9, 1, 0, 7, 12, 0, 2, 12, 10, 1 };
const QVector<int> Wave5Bank31 = {
  2, 1, 14, 2, 3, 3, 2, 8, 14, 1, 2, 2, 15, 15, 2, 2,
  15, 7, 2, 4, 2, 2, 15, 7, 3, 4, 2, 4, 15, 7, 4, 4 };

/// Comments off, ends trimmed. rgbasm's `;` runs to end of line.
QString strip(const QString& line)
{
  const int semi = line.indexOf(QLatin1Char(';'));
  return (semi < 0 ? line : line.left(semi)).trimmed();
}

/// Where the mnemonic ends. ⚠️ **NOT a QRegularExpression.**
///
/// It was `line.indexOf(QRegularExpression("\\s"))` — compiled fresh, on **every line of 375 files**,
/// which is roughly 40,000 regex compilations and made the parse take a **second**. A second is a
/// second: it is a stutter on the first ▶, and it would be a second of boot if we parsed eagerly.
/// A loop over the characters does the same job in nothing at all.
int firstSpace(const QString& s)
{
  for (int i = 0; i < s.size(); i++) {
    if (s.at(i).isSpace())
      return i;
  }

  return -1;
}

/// `$ff` hex, `%1010` binary, plain decimal. Signed decimals happen (`dn` takes them).
bool parseInt(const QString& raw, int& out)
{
  const QString t = raw.trimmed();
  if (t.isEmpty())
    return false;

  bool ok = false;

  if (t.startsWith(QLatin1Char('$')))
    out = t.mid(1).toInt(&ok, 16);
  else if (t.startsWith(QLatin1Char('%')))
    out = t.mid(1).toInt(&ok, 2);
  else
    out = t.toInt(&ok, 10);

  return ok;
}

/// `dn hi, lo` — but a **NEGATIVE low nibble is signed-magnitude**: bit 3 is the sign, not a two's
/// complement. Get this wrong and every fade and every pitch bend in the game is subtly off.
int nibbles(int hi, int lo)
{
  if (lo < 0)
    lo = 0b1000 | (-lo);

  return ((hi & 0xF) << 4) | (lo & 0xF);
}

/// A label's software channel (0-7), out of its name: `Music_PalletTown_Ch3` -> 2.
///
/// ⚠️ Not cosmetic. The channel changes what a byte *means* — `$2x` is an `sfx_note` on CHAN4-CHAN8
/// and an ordinary one-byte `note` on the music tone channels. @see Gen1SoundEngine.
int channelOf(const QString& label)
{
  static const QRegularExpression re(QStringLiteral("_Ch(\\d)"));
  const auto m = re.match(label);
  return m.hasMatch() ? m.captured(1).toInt() - 1 : 0;
}

/// One header entry: `Music_PalletTown:: channel_count 3 / channel 1, Music_PalletTown_Ch1 / ...`
struct HeaderEntry {
  QString label;
  int count = 0;
  QVector<QPair<int, QString>> chans;   ///< (software channel 1-8, target label)
};

QVector<HeaderEntry> parseHeaders(const QString& text)
{
  QVector<HeaderEntry> out;
  HeaderEntry cur;

  const QStringList lines = text.split(QLatin1Char('\n'));

  for (const QString& raw : lines) {
    const QString line = strip(raw);
    if (line.isEmpty())
      continue;

    // A label sits at column 0. (Indented lines are macros.)
    const bool indented = !raw.isEmpty() && (raw.at(0) == QLatin1Char(' ') || raw.at(0) == QLatin1Char('\t'));

    if (line.endsWith(QLatin1String("::")) || (line.endsWith(QLatin1Char(':')) && !indented)) {
      if (!cur.label.isEmpty() && !cur.chans.isEmpty())
        out.append(cur);

      cur = HeaderEntry();
      cur.label = line;
      while (cur.label.endsWith(QLatin1Char(':')))
        cur.label.chop(1);

      continue;
    }

    const int sp = firstSpace(line);
    const QString op = (sp < 0) ? line : line.left(sp);
    const QString rest = (sp < 0) ? QString() : line.mid(sp).trimmed();

    if (op == QLatin1String("channel_count")) {
      parseInt(rest, cur.count);
    }
    else if (op == QLatin1String("channel")) {
      const QStringList a = rest.split(QLatin1Char(','));
      if (a.size() == 2) {
        int c = 0;
        if (parseInt(a.at(0), c))
          cur.chans.append({ c, a.at(1).trimmed() });
      }
    }
  }

  if (!cur.label.isEmpty() && !cur.chans.isEmpty())
    out.append(cur);

  return out;
}

/// Read a file, whether it is on disk or in a Qt resource.
bool readText(const QString& path, QString& out)
{
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  out = QString::fromUtf8(f.readAll());
  return true;
}

} // namespace

// ── The line parser ──────────────────────────────────────────────────────────────────────────────

bool Gen1MusicAsm::op(Stream& s, const QString& op, const QStringList& a, int chan, QString& err)
{
  auto emit1 = [&](int b) { s.data.append(static_cast<char>(b & 0xFF)); };

  // A pointer we cannot fill in yet: reserve two bytes and remember where. Local labels ('.loop')
  // have ALREADY been scoped by assemble(), which is the only place that knows what the enclosing
  // label is.
  auto ptr = [&](const QString& target) {
    s.fixups.append({ static_cast<int>(s.data.size()), target.trimmed() });
    emit1(0);
    emit1(0);
  };

  auto num = [&](int i) {
    int v = 0;
    parseInt(a.value(i), v);
    return v;
  };

  Q_UNUSED(chan)

  // ⚠️ THE MACRO TABLE. This is the whole "compiler", and it is a table because **their macro names
  // ARE the command names** (macros/scripts/audio.asm). That is Twilight's insight, and it is why
  // this file is 200 lines instead of a rgbasm port.
  if (op == QLatin1String("note")) {
    const int n = noteIndex(a.value(0).trimmed());
    if (n < 0) { err = QStringLiteral("unknown note '%1'").arg(a.value(0)); return false; }
    emit1((n << 4) | (num(1) - 1));
  }
  else if (op == QLatin1String("rest")) {
    emit1(0xC0 | (num(0) - 1));
  }
  else if (op == QLatin1String("octave")) {
    emit1(0xE0 | (8 - num(0)));
  }
  else if (op == QLatin1String("note_type")) {
    emit1(0xD0 | num(0));
    emit1(nibbles(num(1), num(2)));
  }
  else if (op == QLatin1String("drum_speed")) {
    emit1(0xD0 | num(0));
  }
  else if (op == QLatin1String("drum_note")) {
    emit1(0xB0 | (num(1) - 1));
    emit1(num(0));
  }
  else if (op == QLatin1String("duty_cycle")) {
    emit1(0xEC);
    emit1(num(0));
  }
  else if (op == QLatin1String("duty_cycle_pattern")) {
    emit1(0xFC);
    emit1((num(0) << 6) | (num(1) << 4) | (num(2) << 2) | num(3));
  }
  else if (op == QLatin1String("tempo")) {
    const int t = num(0);
    emit1(0xED);
    emit1((t >> 8) & 0xFF);
    emit1(t & 0xFF);
  }
  else if (op == QLatin1String("volume")) {
    emit1(0xF0);
    emit1((num(0) << 4) | num(1));
  }
  else if (op == QLatin1String("stereo_panning")) {
    emit1(0xEE);
    emit1((num(0) << 4) | num(1));
  }
  else if (op == QLatin1String("vibrato")) {
    emit1(0xEA);
    emit1(num(0));
    emit1((num(1) << 4) | num(2));
  }
  else if (op == QLatin1String("toggle_perfect_pitch")) {
    emit1(0xE8);
  }
  else if (op == QLatin1String("pitch_slide")) {
    emit1(0xEB);
    emit1(num(0) - 1);

    // The target: an octave and either a NOTE NAME or a raw number. Both forms occur.
    const QString t = a.value(2).trimmed();
    const int n = noteIndex(t);
    int raw = 0;
    if (n < 0)
      parseInt(t, raw);

    emit1(((8 - num(1)) << 4) | (n >= 0 ? n : raw));
  }
  else if (op == QLatin1String("pitch_sweep")) {
    emit1(0x10);
    emit1(nibbles(num(0), num(1)));
  }
  else if (op == QLatin1String("square_note")) {
    emit1(0x20 | num(0));
    emit1(nibbles(num(1), num(2)));
    const int f = num(3);
    emit1(f & 0xFF);
    emit1((f >> 8) & 0xFF);
  }
  else if (op == QLatin1String("noise_note")) {
    emit1(0x20 | num(0));
    emit1(nibbles(num(1), num(2)));
    emit1(num(3));
  }
  else if (op == QLatin1String("execute_music")) {
    emit1(0xF8);
  }
  else if (op == QLatin1String("unknownmusic0xef")) {
    emit1(0xEF);
    emit1(num(0));
  }
  else if (op == QLatin1String("sound_call")) {
    emit1(0xFD);
    ptr(a.value(0));
  }
  else if (op == QLatin1String("sound_loop")) {
    emit1(0xFE);
    emit1(num(0));
    ptr(a.value(1));
  }
  else if (op == QLatin1String("sound_ret")) {
    emit1(0xFF);
  }
  else if (op == QLatin1String("dn")) {
    for (int i = 0; i + 1 < a.size(); i += 2)
      emit1(nibbles(num(i), num(i + 1)));
  }
  else if (op == QLatin1String("db")) {
    for (int i = 0; i < a.size(); i++)
      emit1(num(i));
  }
  else if (op == QLatin1String("dw") || op == QLatin1String("channel_count")
           || op == QLatin1String("channel") || op == QLatin1String("INCLUDE")
           || op == QLatin1String("SECTION") || op == QLatin1String("table_width")
           || op == QLatin1String("assert_table_length")) {
    // Not part of a command stream.
  }
  else {
    err = QStringLiteral("unknown macro '%1'").arg(op);
    return false;
  }

  return true;
}

bool Gen1MusicAsm::assemble(const QString& path, const QString& text, Stream& out, QString& err)
{
  out.file = path;

  QString scope;
  int chan = 0;

  // ⚠️ A couple of sound effects differ between Red and Blue (`IF DEF(_RED)`). This editor is for the
  // **US English RED** cartridge, so we take the `_RED` branch -- the same one the emulator parity
  // test's cartridge is.
  bool emitting = true;

  const QStringList lines = text.split(QLatin1Char('\n'));

  for (const QString& raw : lines) {
    const QString line = strip(raw);
    if (line.isEmpty())
      continue;

    if (line.startsWith(QLatin1String("IF "))) {
      emitting = line.contains(QLatin1String("DEF(_RED)"));
      continue;
    }
    if (line == QLatin1String("ELSE")) {
      emitting = !emitting;
      continue;
    }
    if (line == QLatin1String("ENDC")) {
      emitting = true;
      continue;
    }
    if (!emitting)
      continue;

    // ⚠️ LABELS SIT AT COLUMN 0; MACROS ARE INDENTED. Getting this wrong makes a bare command like
    // `sound_ret` look like a label, which silently wrecks every scope after it -- and the music
    // still "works", it just plays the wrong notes. (The Python importer hit this; its id-resolution
    // guard is what caught it.)
    const bool indented = (raw.at(0) == QLatin1Char(' ') || raw.at(0) == QLatin1Char('\t'));

    if (!indented) {
      QString name = line;
      while (name.endsWith(QLatin1Char(':')))
        name.chop(1);

      if (name.startsWith(QLatin1Char('.'))) {
        out.labels.insert(scope + name, static_cast<int>(out.data.size()));
      }
      else {
        scope = name;
        out.labels.insert(name, static_cast<int>(out.data.size()));
        chan = channelOf(name);
      }

      continue;
    }

    const int sp = firstSpace(line);
    const QString mnemonic = (sp < 0) ? line : line.left(sp);
    const QString rest = (sp < 0) ? QString() : line.mid(sp).trimmed();

    QStringList args;
    if (!rest.isEmpty()) {
      for (const QString& piece : rest.split(QLatin1Char(',')))
        args.append(piece.trimmed());
    }

    // A local pointer target ('.loop') is scoped to the enclosing label. Resolve it here, while we
    // still know what that is -- `op()` no longer does.
    QStringList scoped = args;
    for (QString& t : scoped) {
      if (t.startsWith(QLatin1Char('.')))
        t = scope + t;
    }

    if (!Gen1MusicAsm::op(out, mnemonic, scoped, chan, err)) {
      err = QStringLiteral("%1: %2").arg(path, err);
      return false;
    }
  }

  return true;
}

// ── Laying the bank out ──────────────────────────────────────────────────────────────────────────

Gen1MusicAsm::Result Gen1MusicAsm::parse(const QString& root)
{
  Result r;

  auto fail = [&](const QString& why) {
    r.ok = false;
    r.error = why;
    return r;
  };

  // ── every command-stream source, assembled once ─────────────────────────────────────────────
  //
  // ⚠️ ALL of them -- music AND sfx. The SFX headers come FIRST in the id table, so a sound effect we
  // failed to place would shift every music id after it. And a shifted id is a different song.
  QHash<QString, Stream> streams;
  QStringList order;   // ⚠️ THE ORDER MATTERS -- see ownerOf below. A QHash has none.

  for (const QString& sub : { QStringLiteral("music"), QStringLiteral("sfx") }) {
    const QDir dir(root + QLatin1Char('/') + sub);
    const QStringList files = dir.entryList({ QStringLiteral("*.asm") }, QDir::Files, QDir::Name);

    if (files.isEmpty())
      return fail(QStringLiteral("no .asm under %1/%2").arg(root, sub));

    for (const QString& f : files) {
      QString text;
      if (!readText(dir.filePath(f), text))
        return fail(QStringLiteral("cannot read %1").arg(dir.filePath(f)));

      Stream s;
      QString err;
      if (!assemble(f, text, s, err))
        return fail(err);

      streams.insert(f, s);
      order.append(f);
    }
  }

  // ⚠️ label -> the stream that defines it, **FIRST DEFINITION WINS** -- and "first" has to mean
  // something, so it is walked in `order` (music then sfx, each sorted by name), NOT in QHash order.
  //
  // Iterating the QHash here would pick an arbitrary owner whenever two files define the same label,
  // and the layout -- and therefore every id after it -- would change from run to run. A different
  // id is a different song. This is the same order the Python importer uses.
  QHash<QString, QString> ownerOf;
  for (const QString& f : order) {
    const Stream& s = streams[f];
    for (auto l = s.labels.constBegin(); l != s.labels.constEnd(); ++l) {
      if (!ownerOf.contains(l.key()))
        ownerOf.insert(l.key(), f);
    }
  }

  struct BankSrc { int id; const char* sfxh; const char* mush; };
  const QVector<BankSrc> BANKS = {
    { 2,  "sfxheaders1.asm", "musicheaders1.asm" },
    { 8,  "sfxheaders2.asm", "musicheaders2.asm" },
    { 31, "sfxheaders3.asm", "musicheaders3.asm" },
  };

  for (const BankSrc& b : BANKS) {
    QString sfxText;
    QString musText;

    if (!readText(root + QStringLiteral("/headers/") + QLatin1String(b.sfxh), sfxText)
        || !readText(root + QStringLiteral("/headers/") + QLatin1String(b.mush), musText))
      return fail(QStringLiteral("cannot read bank %1's headers").arg(b.id));

    const QVector<HeaderEntry> sfx = parseHeaders(sfxText);
    QVector<HeaderEntry> entries = sfx;
    entries += parseHeaders(musText);

    if (sfx.isEmpty() || entries.isEmpty())
      return fail(QStringLiteral("bank %1: no headers").arg(b.id));

    // ── THE HEADER TABLE. 3 bytes per CHANNEL. Id 0 is the $ff $ff $ff padding. ─────────────
    //
    // This table's LAYOUT is load-bearing: an id is (address - table start) / 3, so one entry out of
    // place shifts every id after it -- and every one of those is somebody's song.
    QByteArray table(3, static_cast<char>(0xFF));
    QVector<QPair<int, QString>> tableFixups;   // (offset in table, target label)
    QHash<QString, int> idOf;

    for (const HeaderEntry& e : entries) {
      idOf.insert(e.label, table.size() / 3);

      for (int i = 0; i < e.chans.size(); i++) {
        const bool first = (i == 0);
        const int c = e.chans.at(i).first;

        table.append(static_cast<char>((first ? ((e.count - 1) << 6) : 0) | (c - 1)));
        tableFixups.append({ table.size(), e.chans.at(i).second });
        table.append(2, '\0');
      }
    }

    if (table.size() > 256 * 3)
      return fail(QStringLiteral("bank %1: header table is %2 bytes -- over 256 ids")
                    .arg(b.id).arg(table.size()));

    table.append(256 * 3 - table.size(), static_cast<char>(0xFF));

    // ── the streams, laid out after the table, reachable-first ───────────────────────────────
    QByteArray blob = table;

    QHash<QString, int> placed;   // file -> base address
    QVector<QPair<int, QString>> pending = tableFixups;

    // A handful of sound effects live in .asm files that are CODE, not data (poke_flute.asm,
    // low_health_alarm.asm, pokedex_rating_sfx.asm -- they poke the hardware directly). We do not
    // import those, so their header entries point at a lone `sound_ret`: **the id stays valid and
    // simply plays nothing.** Music is unaffected, and that is the point -- the ids must not shift.
    const int stub = Base + blob.size();
    blob.append(static_cast<char>(0xFF));

    std::function<void(const QString&)> place = [&](const QString& file) {
      const Stream& s = streams[file];
      placed.insert(file, Base + blob.size());
      blob.append(s.data);

      for (const auto& f : s.fixups)
        pending.append({ placed[file] - Base + f.first, f.second });
    };

    auto addrOf = [&](const QString& label) -> int {
      const QString file = ownerOf.value(label);
      if (file.isEmpty())
        return stub;

      if (!placed.contains(file))
        place(file);

      return placed[file] + streams[file].labels.value(label);
    };

    for (int i = 0; i < pending.size(); i++) {          // `pending` GROWS as we place streams
      const int off = pending.at(i).first;
      const int addr = addrOf(pending.at(i).second);

      blob[off] = static_cast<char>(addr & 0xFF);
      blob[off + 1] = static_cast<char>((addr >> 8) & 0xFF);
    }

    Bank out;
    out.data.assign(blob.constBegin(), blob.constEnd());

    // MAX_SFX_ID_N -- the id of the LAST sound-effect header. PlaySound branches on it: at or below,
    // the SFX path (a partial init); above, the MUSIC path (a full re-init).
    out.maxSfxId = idOf.value(sfx.last().label);

    if (b.id == 2)       r.bank2 = out;
    else if (b.id == 8)  r.bank8 = out;
    else                 r.bank31 = out;
  }

  // ── the wave instruments ────────────────────────────────────────────────────────────────────
  QString waveText;
  if (!readText(root + QStringLiteral("/wave_samples.asm"), waveText))
    return fail(QStringLiteral("cannot read wave_samples.asm"));

  for (int i = 0; i < 5; i++) {
    const QRegularExpression re(
      QStringLiteral("^\\.wave%1\\s*\\n\\s*dn (.+)$").arg(i),
      QRegularExpression::MultilineOption);

    const auto m = re.match(waveText);
    if (!m.hasMatch())
      return fail(QStringLiteral("wave_samples.asm: no .wave%1").arg(i));

    QVector<int> vals;
    for (const QString& tok : strip(m.captured(1)).split(QLatin1Char(','))) {
      int v = 0;
      if (!parseInt(tok, v))
        return fail(QStringLiteral("wave_samples.asm: bad nibble '%1'").arg(tok));
      vals.append(v);
    }

    for (int j = 0; j + 1 < vals.size(); j += 2)
      r.waves.push_back(static_cast<uint8_t>((vals.at(j) << 4) | vals.at(j + 1)));
  }

  // ...and `.wave5`, once per bank. @see the note on Wave5Bank2 above -- it is a BUG, and it is
  // Lavender Town's instrument.
  for (const QVector<int>* w : { &Wave5Bank2, &Wave5Bank8, &Wave5Bank31 }) {
    for (int j = 0; j + 1 < w->size(); j += 2)
      r.waves.push_back(static_cast<uint8_t>((w->at(j) << 4) | w->at(j + 1)));
  }

  r.ok = true;
  return r;
}
