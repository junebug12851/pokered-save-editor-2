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

/**
 * @file tst_player.cpp
 * @brief The player's 26-byte map-state block -- the Details panel's content when he is selected.
 *
 * The domain write-up is notes/reference/player-state.md, and it is verified against the cartridge
 * (`scripts/emu/probe_player_state.py`). This file pins the MODEL to it.
 *
 * The keystone is `setPlayerField_writesExactlyOneByteOrBit`: set every field the panel offers,
 * byte-diff the WHOLE 32 KB save, and demand that only the intended byte (or, for a flag, only the
 * intended bit of its shared byte) moved. Byte-fidelity is a top-tier project value.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"
#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

namespace {

/// One field the panel offers: its key, the save byte it lives in, and (for a flag) which bit --
/// straight out of notes/reference/player-state.md. `bit == -1` means the whole byte is the field.
struct Field {
  const char* key;
  int offset;
  int bit;      // -1 for a whole-byte field
  bool reload;  // ⚠️ rewritten on load?
  bool dead;    // 💀 written, never read?
};

// Durable fields, then the ten rewritten-on-load, then the three dead. Offsets are WRAM - 0xAD54.
const QVector<Field> kFields = {
  { "moveDir",               0x27D4, -1, false, false },
  { "lastStopDir",           0x27D5, -1, false, false },
  { "walkBikeSurf",          0x29AC, -1, false, false },
  { "xBlockCoord",           0x2610, -1, false, false },
  { "yBlockCoord",           0x260F, -1, false, false },
  { "surfingAllowed",        0x29D4,  1, false, false },
  { "arrivedByFly",          0x29DF,  7, false, false },
  { "noBattles",             0x29DA,  4, false, false },
  { "standingOnWarp",        0x29E2,  2, false, false },
  { "spinPlayer",            0x29E2,  7, false, false },

  { "curDir",                0x27D6, -1, true,  false },   // forced DOWN on load
  { "strengthOutsideBattle", 0x29D4,  0, true,  false },   // reset on load
  { "isBattle",              0x29D9,  6, true,  false },   // wStatusFlags3 zeroed
  { "isTrainerBattle",       0x29D9,  7, true,  false },
  { "battleEndedOrBlackout", 0x29DA,  5, true,  false },   // cleared on entry
  { "usingLinkCable",        0x29DA,  6, true,  false },
  { "standingOnDoor",        0x29E2,  0, true,  false },
  { "movingThroughDoor",     0x29E2,  1, true,  false },
  { "finalLedgeJumping",     0x29E2,  6, true,  false },
  { "jumpingY",              0x29C0, -1, true,  false },   // zeroed on load

  { "usedCardKey",           0x29D4,  7, false, true  },   // BIT_UNUSED_CARD_KEY -- never checked
  { "xOffsetSpecialWarp",    0x278F, -1, false, true  },
  { "yOffsetSpecialWarp",    0x278E, -1, false, true  },
};

QVariantMap fieldNamed(const QVariantList& fields, const QString& key)
{
  for (const QVariant& v : fields) {
    const QVariantMap m = v.toMap();
    if (m.value("key").toString() == key)
      return m;
  }
  return QVariantMap();
}

} // namespace

class TestPlayer : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void playerFields_nameEveryByteInEnglish();
  void rewrittenAndDeadFields_areAbsentUntilYouAskForThem();
  void reloadAndDead_areMarkedAsDifferentFacts();
  void setPlayerField_writesExactlyOneByteOrBit();
  void loadingAndResavingAnUntouchedSave_changesNothing();

private:
  QByteArray m_orig;

  struct Rig {
    SaveFile sf;
    MapModel* map = nullptr;
  };

  Rig* makeRig()
  {
    auto* r = new Rig;
    loadInto(r->sf, m_orig);

    auto* area = r->sf.dataExpanded->area;
    r->map = new MapModel(area->map, area->player, area->tileset, area->general,
                          area->preloadedSprites, area->sprites, area->warps,
                          r->sf.dataExpanded->world->general);
    return r;
  }

  static QString describeDiff(const QVector<int>& moved)
  {
    QStringList s;
    for (int i : moved)
      s << QStringLiteral("0x%1").arg(i, 4, 16, QChar('0'));
    return s.join(QStringLiteral(", "));
  }
};

void TestPlayer::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// Every field the panel offers comes back with a real English label and a sentence saying what it
/// is. Nobody should have to already know what `wStatusFlags1` bit 0 means.
void TestPlayer::playerFields_nameEveryByteInEnglish()
{
  Rig* r = makeRig();
  r->map->setShowScratch(true);   // the rewrite/dead fields are only emitted with the switch on

  const QVariantList fields = r->map->playerFields();
  QCOMPARE(fields.size(), kFields.size());

  for (const Field& f : kFields) {
    const QVariantMap m = fieldNamed(fields, f.key);
    QVERIFY2(!m.isEmpty(), qPrintable(QStringLiteral("field missing: %1").arg(f.key)));
    QVERIFY2(!m.value("label").toString().isEmpty(),
             qPrintable(QStringLiteral("field has no label: %1").arg(f.key)));
    QVERIFY2(!m.value("blurb").toString().isEmpty(),
             qPrintable(QStringLiteral("field has no blurb: %1").arg(f.key)));
    QVERIFY(!m.value("group").toString().isEmpty());
  }

  delete r;
}

/// ⚠️💀 The ten the game rewrites on load and the three it never reads are CLUTTER above the fields
/// that do something -- so they are absent until the "Reloaded values" switch is on. Filtered in the
/// MODEL, so no view can leak one.
void TestPlayer::rewrittenAndDeadFields_areAbsentUntilYouAskForThem()
{
  Rig* r = makeRig();

  // OFF: only the durable fields.
  r->map->setShowScratch(false);
  const QVariantList off = r->map->playerFields();
  for (const Field& f : kFields) {
    const bool present = !fieldNamed(off, f.key).isEmpty();
    if (f.reload || f.dead)
      QVERIFY2(!present, qPrintable(QStringLiteral("%1 leaked while the switch was off").arg(f.key)));
    else
      QVERIFY2(present, qPrintable(QStringLiteral("durable field %1 went missing").arg(f.key)));
  }

  // ON: all of them.
  r->map->setShowScratch(true);
  const QVariantList on = r->map->playerFields();
  for (const Field& f : kFields)
    QVERIFY2(!fieldNamed(on, f.key).isEmpty(),
             qPrintable(QStringLiteral("%1 absent even with the switch on").arg(f.key)));

  delete r;
}

/// A byte the console REWRITES on load and one it never READS are two different facts, and the panel
/// must say which. Each rewrite field wears mark "reload" with its own explanation; each dead one
/// wears "dead". They never share a mark.
void TestPlayer::reloadAndDead_areMarkedAsDifferentFacts()
{
  Rig* r = makeRig();
  r->map->setShowScratch(true);
  const QVariantList fields = r->map->playerFields();

  for (const Field& f : kFields) {
    const QVariantMap m = fieldNamed(fields, f.key);
    QVERIFY(!m.isEmpty());

    if (f.reload) {
      QCOMPARE(m.value("mark").toString(), QStringLiteral("reload"));
      QVERIFY(m.value("scratch").toBool());
      QVERIFY2(!m.value("note").toString().isEmpty(),
               qPrintable(QStringLiteral("%1 is rewritten on load but carries no explanation").arg(f.key)));
    } else if (f.dead) {
      QCOMPARE(m.value("mark").toString(), QStringLiteral("dead"));
      QVERIFY(m.value("dead").toBool());
      QVERIFY(!m.value("note").toString().isEmpty());
    } else {
      QVERIFY2(m.value("mark").toString().isEmpty(),
               qPrintable(QStringLiteral("durable field %1 wears a mark it shouldn't").arg(f.key)));
    }
  }

  delete r;
}

/**
 * @brief THE KEYSTONE. Set every field the panel offers and the save moves by **exactly one byte** --
 *        or, for a flag, by exactly one BIT of its shared status byte and nothing else.
 *
 * Byte-fidelity is sacred (context/principles.md). A player flag lives in a byte it shares with
 * other systems' flags (STRENGTH sits beside the fishing-rod bits; the door bits sit in a byte the
 * game uses per-step); if setting one of ours disturbed one of theirs, this goes red.
 */
void TestPlayer::setPlayerField_writesExactlyOneByteOrBit()
{
  for (const Field& f : kFields) {
    Rig* r = makeRig();
    r->map->setShowScratch(true);

    r->sf.flattenData();
    const QByteArray before = snapshot(r->sf);

    const int oldByte = static_cast<unsigned char>(before.at(f.offset));

    if (f.bit < 0) {
      // A whole byte: flip it to something guaranteed different, full range allowed.
      r->map->setPlayerField(QString::fromLatin1(f.key), oldByte ^ 0x5A);
    } else {
      // A single bit: flip just that one.
      const int oldBit = (oldByte >> f.bit) & 1;
      r->map->setPlayerField(QString::fromLatin1(f.key), oldBit ? 0 : 1);
    }

    r->sf.flattenData();
    const QByteArray after = snapshot(r->sf);

    QCOMPARE(before.size(), after.size());
    const QVector<int> moved = diffOffsets(before, after);

    const QVector<int> expected = { f.offset };
    QVERIFY2(moved == expected,
             qPrintable(QStringLiteral("setting %1 moved %2 bytes, not just 0x%3: %4")
                          .arg(f.key)
                          .arg(moved.size())
                          .arg(f.offset, 4, 16, QChar('0'))
                          .arg(describeDiff(moved))));

    // For a flag, prove it was EXACTLY that bit -- the rest of the shared byte is untouched.
    if (f.bit >= 0) {
      const int delta = static_cast<unsigned char>(before.at(f.offset))
                      ^ static_cast<unsigned char>(after.at(f.offset));
      QCOMPARE(delta, 1 << f.bit);
    }

    delete r;
  }
}

/// Loading a save and flattening it again, having touched nothing, changes not one byte. The player
/// block round-trips through load()/save() with perfect fidelity.
void TestPlayer::loadingAndResavingAnUntouchedSave_changesNothing()
{
  Rig* r = makeRig();

  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);

  r->sf.flattenData();
  const QByteArray after = snapshot(r->sf);

  const QVector<int> moved = diffOffsets(before, after);
  QVERIFY2(moved.isEmpty(),
           qPrintable(QStringLiteral("re-flattening an untouched save moved %1 bytes: %2")
                        .arg(moved.size())
                        .arg(describeDiff(moved))));

  delete r;
}

QTEST_MAIN(TestPlayer)
#include "tst_player.moc"
