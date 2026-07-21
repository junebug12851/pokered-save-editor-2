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
 * @file tst_signs.cpp
 * @brief The map's SIGNS -- the placard list, and the map text behind each one.
 *
 * The domain write-up is notes/reference/signs.md. The save model was already correct (the rare pass
 * with no bug to fix first); what this pins is (a) byte-fidelity through the canvas machinery and
 * (b) that the imported text table (scripts/import_sign_text.py) resolves every shipped sign.
 *
 * Keystones:
 *  - `moveSign_writesExactlyTwoBytes` -- byte-diff the WHOLE 32 KB save across a drag and demand that
 *    the sign's Y and X are the only things in it that moved.
 *  - `everyShippedSignResolvesInItsMapsText` -- every sign in maps.json points at a real entry of its
 *    own map's text table. If the importer or the alignment is ever wrong, this goes red by name.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"
#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentrysign.h>
#include <pse-db/entries/mapdbentrytext.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areasign.h>
#include <pse-savefile/expanded/fragments/signdata.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

namespace {

// The save offsets, straight out of notes/reference/signs.md. (WRAM = save + 0xAD54.)
constexpr int kNumSigns    = 0x275C;  // wNumSigns    $D4B0
constexpr int kSignCoords  = 0x275D;  // wSignCoords  $D4B1  (Y, X) x16
constexpr int kSignTextIDs = 0x277D;  // wSignTextIDs $D4D1  (txtId) x16

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

class TestSigns : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void signList_drawsTheMapsRealSigns();
  void moveSign_writesExactlyTwoBytes();
  void moveSign_clampsToTheMap();
  void addSign_defaultsToRealMapText();
  void removeSign_slidesTheRestUp();
  void signRoomLeft_countsDownFromSixteen();
  void signsEdited_isQuietUntilYouActuallyChangeSomething();

  void signText_resolvesToRealWords();
  void signTextList_isGroupedByCategory();
  void signFields_nameEveryByteInEnglish();
  void outOfRangeTextId_isFlaggedNotRefused();

  void everyShippedSignResolvesInItsMapsText();
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
                          r->sf.dataExpanded->world->general, area->signs);
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

void TestSigns::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// The fixture is standing in a real map with real signs, and every one comes back with a place, a
/// drawable rect, and (for a real one) its words.
void TestSigns::signList_drawsTheMapsRealSigns()
{
  Rig* r = makeRig();

  const QVariantList signs = r->map->signList();
  QVERIFY2(!signs.isEmpty(), "the fixture's map has no signs -- pick a different fixture");

  for (const QVariant& v : signs) {
    const QVariantMap m = v.toMap();

    QVERIFY(m.contains("ind"));
    QVERIFY(m.value("x").toInt() >= 0);
    QVERIFY(m.value("y").toInt() >= 0);

    QVERIFY(m.value("rectX").toInt() > 0);
    QVERIFY(m.value("rectY").toInt() > 0);
    QCOMPARE(m.value("rectW").toInt(), 16);   // a sign is a TILE, not a block
    QCOMPARE(m.value("rectH").toInt(), 16);
  }

  delete r;
}

/**
 * @brief THE KEYSTONE. Drag a sign across town and the save changes by **exactly two bytes**.
 *
 * Byte-fidelity is a top-tier project value. If this ever goes red, something is rewriting bytes it
 * was never told to touch.
 */
void TestSigns::moveSign_writesExactlyTwoBytes()
{
  Rig* r = makeRig();

  const QVariantMap first = r->map->signList().first().toMap();
  const int ind = first.value("ind").toInt();
  const int cx  = first.value("x").toInt();
  const int cy  = first.value("y").toInt();

  // A small interior target that is guaranteed to differ from the current tile in BOTH coordinates,
  // so both bytes are expected to move (and a clamp cannot mask a miss).
  const int tx = (cx >= 2) ? 1 : 3;
  const int ty = (cy >= 2) ? 1 : 3;

  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);

  r->map->moveSign(ind, tx, ty);

  r->sf.flattenData();
  const QByteArray after = snapshot(r->sf);

  QCOMPARE(before.size(), after.size());

  const QVector<int> moved = diffOffsets(before, after);

  // wSignCoords is { Y, X } per sign -- Y first. Those two bytes, and nothing else in 32 KB.
  const int base = kSignCoords + 2 * ind;
  const QVector<int> expected = { base + 0, base + 1 };

  QVERIFY2(moved == expected,
           qPrintable(QStringLiteral("a drag moved %1 bytes, not just the sign's y/x: %2")
                        .arg(moved.size())
                        .arg(describeDiff(moved))));

  // And it landed where we aimed it.
  const QVariantMap now = r->map->signAt(ind);
  QCOMPARE(now.value("x").toInt(), tx);
  QCOMPARE(now.value("y").toInt(), ty);

  delete r;
}

/// A sign out in the 3-block border ring is one the player can never read. An overshoot stops at the
/// map's edge rather than parking it in the trees.
void TestSigns::moveSign_clampsToTheMap()
{
  Rig* r = makeRig();

  const int w = r->map->blocksWide() * 2;
  const int h = r->map->blocksHigh() * 2;

  r->map->moveSign(0, 9999, 9999);
  QVariantMap s = r->map->signAt(0);
  QCOMPARE(s.value("x").toInt(), w - 1);
  QCOMPARE(s.value("y").toInt(), h - 1);

  r->map->moveSign(0, -50, -50);
  s = r->map->signAt(0);
  QCOMPARE(s.value("x").toInt(), 0);
  QCOMPARE(s.value("y").toInt(), 0);

  delete r;
}

/// A new sign reads real text -- this map's first sign line -- so a fresh placard is not a mystery.
void TestSigns::addSign_defaultsToRealMapText()
{
  Rig* r = makeRig();

  const int ind = r->map->addSign(4, 5);
  QVERIFY(ind >= 0);

  const QVariantMap s = r->map->signAt(ind);
  QCOMPARE(s.value("x").toInt(), 4);
  QCOMPARE(s.value("y").toInt(), 5);

  QVERIFY2(s.value("textId").toInt() >= 1, "a new sign defaulted to text id 0 (points at nothing)");
  QVERIFY2(s.value("textValid").toBool(),
           "a brand-new sign points past this map's text table");

  delete r;
}

/// The game packs its sign list, so deleting one slides the rest up -- and the count byte follows.
void TestSigns::removeSign_slidesTheRestUp()
{
  Rig* r = makeRig();

  const QVariantList before = r->map->signList();
  QVERIFY(before.size() >= 2);

  const QVariantMap second = before.at(1).toMap();
  const int n = before.size();

  r->map->removeSign(0);

  const QVariantList after = r->map->signList();
  QCOMPARE(after.size(), n - 1);

  const QVariantMap nowFirst = after.first().toMap();
  QCOMPARE(nowFirst.value("x").toInt(), second.value("x").toInt());
  QCOMPARE(nowFirst.value("y").toInt(), second.value("y").toInt());
  QCOMPARE(nowFirst.value("textId").toInt(), second.value("textId").toInt());

  r->sf.flattenData();
  QCOMPARE(static_cast<int>(snapshot(r->sf).at(kNumSigns)), n - 1);

  delete r;
}

/// The cap is stated *before* you hit it, not after -- so the model has to be able to say it.
void TestSigns::signRoomLeft_countsDownFromSixteen()
{
  Rig* r = makeRig();

  const int used = r->map->signList().size();
  QCOMPARE(r->map->signRoomLeft(), 16 - used);

  while (r->map->signRoomLeft() > 0)
    QVERIFY(r->map->addSign(1, 1) >= 0);

  QCOMPARE(r->map->signList().size(), 16);
  QCOMPARE(r->map->signRoomLeft(), 0);

  // The 17th is refused, not silently swallowed into a slot the game cannot read.
  QCOMPARE(r->map->addSign(2, 2), -1);
  QCOMPARE(r->map->signList().size(), 16);

  delete r;
}

/// Quiet until *you* change something -- the same rule as warps/cast (track the EDIT, never a diff).
void TestSigns::signsEdited_isQuietUntilYouActuallyChangeSomething()
{
  Rig* r = makeRig();

  QVERIFY2(!r->map->signsEdited(), "the panel is warning about an edit nobody made");

  (void)r->map->signList();
  (void)r->map->signFields(0);
  (void)r->map->signTextList();
  QVERIFY(!r->map->signsEdited());

  const QVariantMap first = r->map->signList().first().toMap();
  r->map->moveSign(0, first.value("x").toInt() >= 2 ? 1 : 3, first.value("y").toInt());

  QVERIFY2(r->map->signsEdited(), "a drag did not trip the note");

  delete r;
}

/// A sign's words come from the map's text table -- and they are real, readable text, not an id.
void TestSigns::signText_resolvesToRealWords()
{
  Rig* r = makeRig();

  bool anyResolved = false;
  for (const QVariant& v : r->map->signList()) {
    const QVariantMap m = v.toMap();
    if (m.value("textValid").toBool() && !m.value("preview").toString().isEmpty())
      anyResolved = true;
  }

  QVERIFY2(anyResolved,
           "not one of the fixture map's signs resolved to real words -- the text table is missing");

  delete r;
}

/// The picker is grouped -- Signs first, then People, then Other -- and each section carries a header,
/// exactly the "select from the text on the map, grouped" project leadership asked for.
void TestSigns::signTextList_isGroupedByCategory()
{
  Rig* r = makeRig();

  const QVariantList list = r->map->signTextList();
  QVERIFY2(!list.isEmpty(), "the fixture map has no text table at all");

  int firstSignAt = -1, firstPersonAt = -1;
  int headerCount = 0;
  bool sawSignCategory = false;

  for (int i = 0; i < list.size(); i++) {
    const QVariantMap m = list.at(i).toMap();
    if (!(m.value("header").toString().isEmpty()))
      headerCount++;

    const QString cat = m.value("category").toString();
    if (cat == "sign") {
      sawSignCategory = true;
      if (firstSignAt < 0) firstSignAt = i;
    } else if (cat == "person" && firstPersonAt < 0) {
      firstPersonAt = i;
    }
  }

  QVERIFY2(sawSignCategory, "no entry is categorised as a sign");
  QVERIFY2(headerCount >= 1, "the grouped list carries no section headers");

  // Signs come before People (when the map has both).
  if (firstSignAt >= 0 && firstPersonAt >= 0)
    QVERIFY2(firstSignAt < firstPersonAt, "People are listed before Signs");

  delete r;
}

/// Two fields, named in English: a position control and the grouped text picker with real options.
void TestSigns::signFields_nameEveryByteInEnglish()
{
  Rig* r = makeRig();

  const QVariantList fields = r->map->signFields(0);
  QCOMPARE(fields.size(), 2);

  const QVariantMap xy = fieldNamed(fields, "xy");
  QVERIFY2(!xy.isEmpty(), "no position field");
  QCOMPARE(xy.value("kind").toString(), QStringLiteral("coords"));
  QVERIFY(!xy.value("label").toString().isEmpty());

  const QVariantMap text = fieldNamed(fields, "textId");
  QVERIFY2(!text.isEmpty(), "no text field");
  QCOMPARE(text.value("kind").toString(), QStringLiteral("enum"));
  QVERIFY(!text.value("label").toString().isEmpty());
  QVERIFY(!text.value("blurb").toString().isEmpty());
  QVERIFY2(!text.value("options").toList().isEmpty(), "the text picker has no options");

  delete r;
}

/// A text id past the map's table is TAKEN and FLAGGED -- never refused, never rewritten.
void TestSigns::outOfRangeTextId_isFlaggedNotRefused()
{
  Rig* r = makeRig();

  MapDBEntry* m = MapsDB::inst()->getStoreAt(r->map->mapInd());
  QVERIFY(m != nullptr);
  const int past = m->getTextEntriesSize() + 50;   // comfortably past this map's text

  r->map->setSignField(0, "textId", past);

  const QVariantMap s = r->map->signAt(0);
  QCOMPARE(s.value("textId").toInt(), past);              // taken, unmangled
  QVERIFY2(!s.value("textValid").toBool(), "an out-of-range text id was not flagged");
  QVERIFY(s.value("preview").toString().isEmpty());       // nothing to resolve

  delete r;
}

/**
 * @brief DATA KEYSTONE. Every sign in maps.json points at a real entry of its own map's text table.
 *
 * This is the model-level twin of the importer's own self-check. If the ind->map alignment ever
 * slips (it did, once -- the unused map slots) or the extraction misses a map, a sign here will land
 * outside its table and this fails BY NAME.
 */
void TestSigns::everyShippedSignResolvesInItsMapsText()
{
  int checked = 0;

  for (int i = 0; i < MapsDB::inst()->getStoreSize(); i++) {
    MapDBEntry* m = MapsDB::inst()->getStoreAt(i);
    if (m == nullptr)
      continue;

    const int textCount = m->getTextEntriesSize();

    for (int s = 0; s < m->getSignsSize(); s++) {
      const MapDBEntrySign* sign = m->getSignsAt(s);
      if (sign == nullptr)
        continue;

      const int id = sign->getTextID();
      QVERIFY2(id >= 1 && id <= textCount,
               qPrintable(QStringLiteral("map %1 (%2): sign text id %3 is outside its text table of %4")
                            .arg(i).arg(m->getName()).arg(id).arg(textCount)));
      checked++;
    }
  }

  QVERIFY2(checked > 0, "no signs were checked -- the map DB has no sign data at all");
}

/// Load an untouched save, re-save it, and not one bit moves.
void TestSigns::loadingAndResavingAnUntouchedSave_changesNothing()
{
  Rig* r = makeRig();

  r->sf.flattenData();
  const QByteArray once = snapshot(r->sf);

  r->sf.expandData();
  r->sf.flattenData();
  const QByteArray twice = snapshot(r->sf);

  const QVector<int> moved = diffOffsets(once, twice);
  QVERIFY2(moved.isEmpty(),
           qPrintable(QStringLiteral("a load/save round trip of an UNTOUCHED save moved %1 bytes: %2")
                        .arg(moved.size()).arg(describeDiff(moved))));

  delete r;
}

QTEST_MAIN(TestSigns)
#include "tst_signs.moc"
