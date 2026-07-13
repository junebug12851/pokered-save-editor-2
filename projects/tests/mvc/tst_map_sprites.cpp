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

/**
 * @file tst_map_sprites.cpp
 * @brief The map screen's CAST -- everything QML calls to draw, place, move and edit the people
 *        and objects on a map.
 *
 * The keystone here is `moveNpc_writesExactlyTwoBytes`: it byte-diffs the WHOLE save across a
 * drag and demands that exactly `mapX` and `mapY` moved. Dragging a person across a town must not
 * disturb one other bit of the file -- that is the project's top-tier value, and this is where a
 * regression in it would show up first.
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
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/fragments/spritedata.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

class TestMapSprites : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void npcList_drawsTheMapsRealCast();
  void npcsEdited_isQuietUntilYouActuallyChangeSomething();
  void moveNpc_writesExactlyTwoBytes();
  void moveNpc_clampsToTheMap();
  void addNpc_thenRemove_slidesTheRestUp();
  void npcFields_coverEveryByteAndAreGrouped();
  void setNpcField_writesTheByteAndTakesHackValues();
  void spriteCatalog_isAllSeventyTwoInGroupOrder();

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
                          area->preloadedSprites, area->sprites);
    return r;
  }
};

void TestMapSprites::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

/// The fixture save is standing in a real map with a real cast, and every one of them comes back
/// with a name, a place and a picture.
void TestMapSprites::npcList_drawsTheMapsRealCast()
{
  Rig* r = makeRig();

  const QVariantList list = r->map->npcList();
  QVERIFY2(!list.isEmpty(), "the fixture's map has no sprites -- pick a different fixture");

  for (const QVariant& v : list) {
    const QVariantMap m = v.toMap();

    QVERIFY(m.value("slot").toInt() >= 1);          // slot 0 is the player, and has his own layer
    QVERIFY(m.value("picture").toInt() >= 1);       // 0 means "unused" and must never be listed
    QVERIFY(!m.value("name").toString().isEmpty());
    QVERIFY(m.value("source").toString().startsWith("image://player/npc/"));

    // The +4 bias is taken off for us -- these are the numbers a player would use.
    QVERIFY(m.value("x").toInt() >= 0);
    QVERIFY(m.value("y").toInt() >= 0);
  }

  delete r;
}

/**
 * ⚠️ THE ONE THAT CAUGHT A DESIGN BUG.
 *
 * The Details panel owes the user a sentence: *the game rebuilds this map's original cast from the
 * cartridge the moment the player walks back in.* The first version decided when to say it by
 * comparing the save's cast against the ROM's -- and that is **wrong**.
 *
 * A real save's cast **already** differs from the ROM's, because WALK sprites wander. Pallet Town's
 * Girl is at (3, 8) in the cartridge and at (3, 6) in this very fixture: she had taken a couple of
 * steps before the game was saved. Comparing would have flashed the warning on essentially every
 * save anyone ever opened -- noise, and noise is a bug.
 *
 * So the panel tracks the EDIT, not the difference: quiet until *you* change something.
 */
void TestMapSprites::npcsEdited_isQuietUntilYouActuallyChangeSomething()
{
  Rig* r = makeRig();

  QVERIFY2(!r->map->npcsEdited(),
           "the panel is warning about an edit nobody made");

  // Merely LOOKING must not trip it.
  (void)r->map->npcList();
  (void)r->map->npcFields(1);
  QVERIFY(!r->map->npcsEdited());

  const QVariantMap first = r->map->npcList().first().toMap();
  r->map->moveNpc(first.value("slot").toInt(),
                  first.value("x").toInt() + 1, first.value("y").toInt());

  QVERIFY2(r->map->npcsEdited(), "a drag did not trip the warning");

  delete r;
}

/// THE KEYSTONE. Drag somebody across town and the save must change by exactly two bytes.
void TestMapSprites::moveNpc_writesExactlyTwoBytes()
{
  Rig* r = makeRig();

  const QVariantMap first = r->map->npcList().first().toMap();
  const int slot = first.value("slot").toInt();

  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);

  r->map->moveNpc(slot, first.value("x").toInt() + 3, first.value("y").toInt() + 2);

  r->sf.flattenData();
  const QByteArray after = snapshot(r->sf);

  QCOMPARE(before.size(), after.size());

  const QVector<int> moved = diffOffsets(before, after);

  // mapX and mapY of this slot -- spritestatedata2 fields 4 and 5. Nothing else in 32 KB.
  // (flattenData does NOT touch the checksum; the save pipeline recalcs it afterwards.)
  const int base = 0x2E2C + 0x10 * slot;
  const QVector<int> expected = { base + 4, base + 5 };

  QVERIFY2(moved == expected,
           qPrintable(QStringLiteral("a drag moved %1 bytes, not just mapX/mapY: %2")
                        .arg(moved.size())
                        .arg([&] {
                          QStringList s;
                          for (int i : moved) s << QStringLiteral("0x%1").arg(i, 4, 16, QChar('0'));
                          return s.join(", ");
                        }())));

  delete r;
}

/// A drag that overshoots stops at the edge. Parking somebody in the 3-block border ring means the
/// game never draws them -- a silent way to lose an NPC.
void TestMapSprites::moveNpc_clampsToTheMap()
{
  Rig* r = makeRig();

  const QVariantMap first = r->map->npcList().first().toMap();
  const int slot = first.value("slot").toInt();

  r->map->moveNpc(slot, -50, -50);
  QVariantMap now = r->map->npcAt(slot);
  QCOMPARE(now.value("x").toInt(), 0);
  QCOMPARE(now.value("y").toInt(), 0);

  r->map->moveNpc(slot, 9999, 9999);
  now = r->map->npcAt(slot);
  QCOMPARE(now.value("x").toInt(), r->map->blocksWide() * 2 - 1);
  QCOMPARE(now.value("y").toInt(), r->map->blocksHigh() * 2 - 1);

  delete r;
}

/// Add somebody, then delete somebody in the middle: the rest SLIDE UP. The game packs its sprite
/// slots and so do we.
void TestMapSprites::addNpc_thenRemove_slidesTheRestUp()
{
  Rig* r = makeRig();

  const int before = r->map->npcList().size();
  const int room = r->map->npcRoomLeft();
  QVERIFY(room > 0);

  const int slot = r->map->addNpc(13 /* Girl */, 4, 4);
  QVERIFY(slot > 0);
  QCOMPARE(r->map->npcList().size(), before + 1);
  QCOMPARE(r->map->npcRoomLeft(), room - 1);

  const QVariantMap added = r->map->npcAt(slot);
  QCOMPARE(added.value("picture").toInt(), 13);
  QCOMPARE(added.value("x").toInt(), 4);
  QCOMPARE(added.value("y").toInt(), 4);

  // Now remove the FIRST npc and check the one after it slid up into its place.
  const QVariantList list = r->map->npcList();
  QVERIFY(list.size() >= 2);
  const int secondPicture = list.at(1).toMap().value("picture").toInt();
  const int firstSlot = list.at(0).toMap().value("slot").toInt();

  r->map->removeNpc(firstSlot);

  const QVariantList after = r->map->npcList();
  QCOMPARE(after.size(), before);   // back to where we started
  QCOMPARE(after.first().toMap().value("slot").toInt(), firstSlot);
  QCOMPARE(after.first().toMap().value("picture").toInt(), secondPicture);

  delete r;
}

/// Every byte of a sprite has a home in the panel, in a named group, with a sentence explaining it.
void TestMapSprites::npcFields_coverEveryByteAndAreGrouped()
{
  Rig* r = makeRig();

  const int slot = r->map->npcList().first().toMap().value("slot").toInt();
  const QVariantList fields = r->map->npcFields(slot);

  QVERIFY2(fields.size() >= 25, "a sprite has more bytes than the panel is showing");

  const QStringList groups = { "Who", "Where", "Movement", "What it is", "Animation scratch" };
  QSet<QString> seenGroups;
  QSet<QString> seenKeys;

  for (const QVariant& v : fields) {
    const QVariantMap f = v.toMap();

    const QString group = f.value("group").toString();
    const QString key   = f.value("key").toString();

    QVERIFY2(groups.contains(group),
             qPrintable(QStringLiteral("field '%1' is in unknown group '%2'").arg(key, group)));
    QVERIFY2(!f.value("label").toString().isEmpty(), qPrintable("field " + key + " has no label"));
    QVERIFY2(!f.value("blurb").toString().isEmpty(),
             qPrintable("field " + key + " has no explanation -- nobody should have to already "
                                         "know what it is"));
    QVERIFY2(!seenKeys.contains(key), qPrintable("field " + key + " is listed twice"));

    seenKeys.insert(key);
    seenGroups.insert(group);
  }

  // Every group has somebody in it -- an empty header is furniture.
  for (const QString& g : groups)
    QVERIFY2(seenGroups.contains(g), qPrintable(QStringLiteral("group '%1' is empty").arg(g)));

  // The bytes the 2026-07-13 research added must all be reachable.
  for (const char* key : { "yAdjusted", "xAdjusted", "collisionData",
                           "origFacingDir", "pictureIDCopy",
                           "movementByte", "rangeDirByte", "faceDir", "grassPriority" })
    QVERIFY2(seenKeys.contains(QString::fromLatin1(key)),
             qPrintable(QStringLiteral("the panel has no row for '%1'").arg(key)));

  delete r;
}

/// A field writes its byte -- and takes a value no real game would hold, because refusing one is
/// not our call to make.
void TestMapSprites::setNpcField_writesTheByteAndTakesHackValues()
{
  Rig* r = makeRig();
  const int slot = r->map->npcList().first().toMap().value("slot").toInt();

  r->map->setNpcField(slot, "grassPriority", 0x80);
  QCOMPARE(r->sf.dataExpanded->area->sprites->spriteAt(slot)->grassPriority, 0x80);

  // A hack value. Movement byte 1 has exactly two legal values; $37 is not one of them, and the
  // editor takes it anyway -- shown and flagged in the panel, never refused, never rewritten.
  r->map->setNpcField(slot, "movementByte", 0x37);
  QCOMPARE(r->sf.dataExpanded->area->sprites->spriteAt(slot)->movementByte, 0x37);

  // X and Y are shown WITHOUT the +4 bias and stored WITH it. One conversion, one place.
  r->map->setNpcField(slot, "mapX", 7);
  QCOMPARE(r->sf.dataExpanded->area->sprites->spriteAt(slot)->mapX, 11);
  QCOMPARE(r->map->npcAt(slot).value("x").toInt(), 7);

  // An unknown key writes NOTHING rather than guessing at a byte.
  const int before = r->sf.dataExpanded->area->sprites->spriteAt(slot)->pictureID;
  r->map->setNpcField(slot, "notAField", 99);
  QCOMPARE(r->sf.dataExpanded->area->sprites->spriteAt(slot)->pictureID, before);

  delete r;
}

/// The Characters bar gets all 72, on five shelves, in the order they read.
void TestMapSprites::spriteCatalog_isAllSeventyTwoInGroupOrder()
{
  Rig* r = makeRig();

  const QVariantList cat = r->map->spriteCatalog();
  QCOMPARE(cat.size(), 72);

  const QStringList order = { "Story", "Trainers", "Townsfolk", "Pokemon", "Objects" };
  int lastGroup = -1;

  for (const QVariant& v : cat) {
    const QVariantMap m = v.toMap();

    const int g = order.indexOf(m.value("group").toString());
    QVERIFY2(g >= 0, "a sprite is in a group the bar has no shelf for");
    QVERIFY2(g >= lastGroup, "the catalog is not in group order -- the shelves would interleave");
    lastGroup = g;

    QVERIFY(m.value("ind").toInt() >= 1);
    QVERIFY(!m.value("name").toString().isEmpty());
    QVERIFY(m.value("source").toString().startsWith("image://player/npc/"));
    QVERIFY(m.contains("inSpriteSet"));
  }

  delete r;
}

QTEST_MAIN(TestMapSprites)
#include "tst_map_sprites.moc"
