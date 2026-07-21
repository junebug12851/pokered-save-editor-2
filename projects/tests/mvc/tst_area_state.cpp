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
 * @file tst_area_state.cpp
 * @brief The map's Area-state fields (v1's "Map" page) on MapModel -- the map-details panel content.
 *
 * Domain: notes/reference/area-map-state.md (console-verified). This pins the MODEL to it.
 *
 * Keystone `setAreaField_writesExactlyItsBytes`: set every field the panel offers and byte-diff the
 * WHOLE 32 KB save -- exactly the intended byte(s) (or, for a flag, one BIT of its shared byte) move.
 * Plus the derived view-box sync doctrine: it follows the player, breaks loose, and re-attaches.
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
#include <pse-savefile/expanded/area/areapokemon.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

namespace {

// key, the save byte(s) it lives in, and the kind: bit>=0 a single bit; bit==-1 a whole byte;
// bit==-2 a little-endian word (two bytes). Offsets are WRAM - 0xAD54.
struct AField {
  const char* key;
  QVector<int> offsets;
  int bit;
};

const QVector<AField> kFields = {
  { "alwaysOnBike",    { 0x29DE },          5 },   // BIT_ALWAYS_ON_BIKE
  { "runScriptOnLoad", { 0x29DF },          4 },   // BIT_USE_CUR_MAP_SCRIPT
  { "mapScript",       { 0x2CE5 },         -1 },   // wCurMapScript
  { "viewPtr",         { 0x260B, 0x260C }, -2 },   // wCurrentTileBlockMapViewPointer
  { "vramViewPtr",     { 0x27D2, 0x27D3 }, -2 },   // wMapViewVRAMPointer
  { "cardKeyDoorX",    { 0x29EC },         -1 },   // wCardKeyDoorX
  { "cardKeyDoorY",    { 0x29EB },         -1 },   // wCardKeyDoorY
};

void applySet(MapModel* m, const QString& key, int val)
{
  if (key == "alwaysOnBike")         m->setAlwaysOnBike(val != 0);
  else if (key == "runScriptOnLoad") m->setRunScriptOnLoad(val != 0);
  else if (key == "mapScript")       m->setMapScript(val);
  else if (key == "viewPtr")         m->setViewPtr(val);
  else if (key == "vramViewPtr")     m->setVramViewPtr(val);
  else if (key == "cardKeyDoorX")    m->setCardKeyDoorX(val);
  else if (key == "cardKeyDoorY")    m->setCardKeyDoorY(val);
}

} // namespace

class TestAreaState : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void mapScriptList_isDescriptiveOnAScriptedMap();
  void setAreaField_writesExactlyItsBytes();
  void viewBox_followsThePlayer_breaksLoose_andReattaches();
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
                          r->sf.dataExpanded->world->general, area->signs, area->pokemon);
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

void TestAreaState::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// The fixture is Pallet Town, which has a named script table -- so the picker is descriptive, and the
/// list carries {value,name,hack} entries (the extraction from pret/pokered, via maps.json).
void TestAreaState::mapScriptList_isDescriptiveOnAScriptedMap()
{
  Rig* r = makeRig();

  QVERIFY(r->map->mapHasScriptList());
  const QVariantList list = r->map->mapScriptList();
  QVERIFY(!list.isEmpty());

  // Every entry names a step; the first is the map's default (id 0).
  const QVariantMap first = list.first().toMap();
  QCOMPARE(first.value("value").toInt(), 0);
  QVERIFY(!first.value("name").toString().isEmpty());

  delete r;
}

/**
 * @brief THE KEYSTONE. Set every Area-state field and the save moves by exactly its byte(s) -- or, for
 *        a flag, exactly one BIT of its shared status byte. Byte-fidelity is sacred.
 *
 * `alwaysOnBike` and `runScriptOnLoad` share their bytes with warp/audio/player flags; if setting one
 * of ours disturbed a neighbour, this goes red.
 */
void TestAreaState::setAreaField_writesExactlyItsBytes()
{
  for (const AField& f : kFields) {
    Rig* r = makeRig();

    r->sf.flattenData();
    const QByteArray before = snapshot(r->sf);

    if (f.bit >= 0) {
      const int oldByte = static_cast<unsigned char>(before.at(f.offsets.first()));
      const int oldBit = (oldByte >> f.bit) & 1;
      applySet(r->map, QString::fromLatin1(f.key), oldBit ? 0 : 1);
    } else if (f.bit == -1) {
      const int oldByte = static_cast<unsigned char>(before.at(f.offsets.first()));
      applySet(r->map, QString::fromLatin1(f.key), oldByte ^ 0x5A);
    } else { // -2: a word
      const int lo = static_cast<unsigned char>(before.at(f.offsets.at(0)));
      const int hi = static_cast<unsigned char>(before.at(f.offsets.at(1)));
      applySet(r->map, QString::fromLatin1(f.key), (lo | (hi << 8)) ^ 0x5A5A);
    }

    r->sf.flattenData();
    const QByteArray after = snapshot(r->sf);

    QCOMPARE(before.size(), after.size());
    const QVector<int> moved = diffOffsets(before, after);

    QVERIFY2(moved == f.offsets,
             qPrintable(QStringLiteral("setting %1 moved %2 bytes (%3), expected %4")
                          .arg(f.key)
                          .arg(moved.size())
                          .arg(describeDiff(moved))
                          .arg(describeDiff(f.offsets))));

    if (f.bit >= 0) {
      const int delta = static_cast<unsigned char>(before.at(f.offsets.first()))
                      ^ static_cast<unsigned char>(after.at(f.offsets.first()));
      QCOMPARE(delta, 1 << f.bit);
    }

    delete r;
  }
}

/// The derived-value doctrine, in one test. The fixture loads SYNCED (the game's own pointer matches
/// what the player's coords derive -- the same fact tst_map's viewPointer keystone proves). Moving the
/// player moves the box with him; breaking sync freezes it and lets a raw value stand; re-attaching
/// snaps it back onto him.
void TestAreaState::viewBox_followsThePlayer_breaksLoose_andReattaches()
{
  Rig* r = makeRig();

  QVERIFY(r->map->viewSynced());
  QCOMPARE(r->map->viewPtr(), r->map->viewPtrComputed());

  // Follows him: move the player, the pointer tracks.
  r->map->movePlayer(r->map->playerX() + 2, r->map->playerY() + 1);
  QVERIFY(r->map->viewSynced());
  QCOMPARE(r->map->viewPtr(), r->map->viewPtrComputed());

  // Break loose with a raw value: desynced, and the value stands.
  const int custom = (r->map->viewPtrComputed() ^ 0x3C3C) & 0xFFFF;
  r->map->setViewPtr(custom);
  QVERIFY(!r->map->viewSynced());
  QCOMPARE(r->map->viewPtr(), custom);

  // Moving him now leaves the loose box exactly where it is.
  r->map->movePlayer(r->map->playerX() + 1, r->map->playerY());
  QCOMPARE(r->map->viewPtr(), custom);
  QVERIFY(!r->map->viewSynced());

  // Re-attach: it snaps back onto the player and tracks again.
  r->map->setViewBreakSync(false);
  QVERIFY(r->map->viewSynced());
  QCOMPARE(r->map->viewPtr(), r->map->viewPtrComputed());

  delete r;
}

/// Loading a save and flattening it again, having touched nothing, changes not one byte.
void TestAreaState::loadingAndResavingAnUntouchedSave_changesNothing()
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

QTEST_MAIN(TestAreaState)
#include "tst_area_state.moc"
