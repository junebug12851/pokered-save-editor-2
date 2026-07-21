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
 * @file tst_sprite_data.cpp
 * @brief Coverage of SpriteData -- one on-map sprite/NPC: the split sprite-data
 *        tables (load/saveSpriteData1/2/NPC), the std::optional QML accessors,
 *        missable save/check, the enum random() helpers, toSprite() resolution,
 *        and the full DB-population paths (load(MapDBEntrySprite*), setTo/setToAll,
 *        randomize/randomizeAll) driven over EVERY real map.
 *
 * Method note (important, mirrors tst_map_fragments): the map-DB sprite links
 * (MapDBEntrySprite::getToSprite()/getToItem()/...) are NULL until
 * MapsDB::deepLink() is called -- DB::deepLinkAll() omits it (it's only needed by
 * the not-yet-wired Maps feature). We call it once in initTestCase(). A probe here
 * confirmed that, once linked, all 918 map sprites resolve and setToAll/randomizeAll
 * run clean over all 249 maps -- so the documented "sprite-link crash" is purely the
 * deepLink-not-called landmine (tracked in status.md), not a SpriteData defect.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/sprites.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentrysprite.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/fragments/spritedata.h>

class TestSpriteData : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void reset_blankDefaults();
  void reset_blankNpcSetsOptionalsToZero();
  void optionalAccessors_setGetReset();
  void stepVectors_setGet();
  void enumRandoms_inRange();
  void toSprite_resolvesValidRejectsInvalid();

  void saveLoad_roundTripAllFields();
  void missables_saveThenCheck();

  // The 2026-07-13 corrections -- each one negative-controlled (put the bug back and the
  // case fails by name, with the byte).  See notes/reference/sprites.md -> Part 5.
  void movementByte1_stayIsFF_walkIsFE();
  void grassPriority_80MeansInGrass();
  void movement2_faceAndRangeAreTheSameByte();
  void movement2_neverLandsInTheFacingField();
  void spriteStateTables_roundTripEveryFieldTheGameKeeps();

  // DB-population invariants (need deepLink)
  void mapSprites_allLinksResolve();
  void setToAllAndRandomizeAll_overEveryMap();
  void setTo_nullIsSafeReset();
};

void TestSpriteData::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  // Resolve every map's sprite/item/trainer/pokemon links (see file note).
  MapsDB::inst()->deepLink();
}

void TestSpriteData::reset_blankDefaults()
{
  SpriteData s;                // default ctor -> load(nullptr) -> reset(false)
  QCOMPARE(s.pictureID, 0);
  QCOMPARE(s.movementStatus, (int)SpriteMovementStatus::Ready);
  QCOMPARE(s.imageIndex, 0xFF);
  QCOMPARE(s.faceDir, (int)SpriteFacing::Down);
  QCOMPARE(s.yDisp, 0x8);
  QCOMPARE(s.xDisp, 0x8);
  QCOMPARE(s.mapY, 4);
  QCOMPARE(s.mapX, 4);
  // A sprite you have just placed STANDS where you put it (STAY = 0xFF) and is NOT flagged
  // as standing in grass (0x00). Both of these were the opposite until 2026-07-13.
  QCOMPARE(s.movementByte, 0xFF);
  QCOMPARE(s.grassPriority, 0x00);
  QCOMPARE(s.walkAnimationCounter, 0x10);

  // A non-blank sprite leaves all optional fields unset -> getters return -1.
  QCOMPARE(s.getRangeDirByte(), -1);
  QCOMPARE(s.getTextID(), -1);
  QCOMPARE(s.getTrainerClassOrItemID(), -1);
  QCOMPARE(s.getTrainerSetID(), -1);
  QCOMPARE(s.getMissableIndex(), -1);
}

void TestSpriteData::reset_blankNpcSetsOptionalsToZero()
{
  SpriteData s(true);          // blankNPC -> the four NPC optionals set to 0
  QCOMPARE(s.getRangeDirByte(), 0);
  QCOMPARE(s.getTextID(), 0);
  QCOMPARE(s.getTrainerClassOrItemID(), 0);
  QCOMPARE(s.getTrainerSetID(), 0);
  // missableIndex is always reset (unset) even for a blank NPC.
  QCOMPARE(s.getMissableIndex(), -1);
}

void TestSpriteData::optionalAccessors_setGetReset()
{
  SpriteData s;
  struct Acc {
    void (SpriteData::*set)(int); int (SpriteData::*get)(); void (SpriteData::*rst)();
  };
  const Acc accs[] = {
    { &SpriteData::setRangeDirByte,         &SpriteData::getRangeDirByte,         &SpriteData::resetRangeDirByte },
    { &SpriteData::setTextID,               &SpriteData::getTextID,               &SpriteData::resetTextID },
    { &SpriteData::setTrainerClassOrItemID, &SpriteData::getTrainerClassOrItemID, &SpriteData::resetTrainerClassOrItemID },
    { &SpriteData::setTrainerSetID,         &SpriteData::getTrainerSetID,         &SpriteData::resetTrainerSetID },
    { &SpriteData::setMissableIndex,        &SpriteData::getMissableIndex,        &SpriteData::resetMissableIndex },
  };
  for(const Acc& a : accs) {
    (s.*a.set)(42);
    QCOMPARE((s.*a.get)(), 42);
    (s.*a.rst)();
    QCOMPARE((s.*a.get)(), -1);   // unset reads back as -1
  }
}

void TestSpriteData::stepVectors_setGet()
{
  SpriteData s;
  s.setXStepVector(200);
  s.setYStepVector(7);
  QCOMPARE(s.getXStepVector(), 200);
  QCOMPARE(s.getYStepVector(), 7);
}

void TestSpriteData::enumRandoms_inRange()
{
  for(int i = 0; i < 40; i++) {
    var8 face = SpriteFacing::random();
    QVERIFY(face == 0 || face == 4 || face == 8 || face == 12);

    var8 mob = SpriteMobility::random();
    QVERIFY(mob == 0xFE || mob == 0xFF);

    var8 mv = SpriteMovement::random();
    QVERIFY(mv <= 10);
    QVERIFY(mv != SpriteMovement::UpDown && mv != SpriteMovement::LeftRight);
    QVERIFY(mv != SpriteMovement::StrengthMovement);   // never randomize a sprite into a boulder

    var8 grass = SpriteGrass::random();
    QVERIFY(grass == 0x00 || grass == 0x80);
  }
}

void TestSpriteData::toSprite_resolvesValidRejectsInvalid()
{
  SpriteData s;
  s.pictureID = SpritesDB::inst()->getStoreAt(0)->ind;   // a real sprite id
  QVERIFY(s.toSprite() != nullptr);

  s.pictureID = 999999;                                  // not a real sprite
  QVERIFY(s.toSprite() == nullptr);
}

void TestSpriteData::saveLoad_roundTripAllFields()
{
  SaveFile sf;                 // blank 32 KB buffer + toolset/iterator
  const var8 index = 3;        // NPC slot (>0) so the NPC table round-trips too

  SpriteData a(true);          // blankNPC: the four NPC optionals are engaged
  a.pictureID = 0x12; a.movementStatus = 1; a.imageIndex = 0xFF;
  a.yStepVector = 0; a.yPixels = 0x20; a.xStepVector = 0; a.xPixels = 0x30;
  a.intraAnimationFrameCounter = 0x05; a.animFrameCounter = 0x06; a.faceDir = 0x08;
  a.walkAnimationCounter = 0x10; a.yDisp = 0x08; a.xDisp = 0x08;
  a.mapY = 0x09; a.mapX = 0x07; a.movementByte = 0xFE; a.grassPriority = 0x80;
  a.movementDelay = 0x00; a.imageBaseOffset = 0x44;
  a.setRangeDirByte(0x03); a.setTextID(0x21);
  a.setTrainerClassOrItemID(0x34); a.setTrainerSetID(0x56);

  a.save(&sf, index);

  SpriteData b(false, &sf, index);   // load it back from the same save slot
  QCOMPARE(b.pictureID, 0x12);
  QCOMPARE(b.movementStatus, 1);
  QCOMPARE(b.imageIndex, 0xFF);
  QCOMPARE(b.yPixels, 0x20);
  QCOMPARE(b.xPixels, 0x30);
  QCOMPARE(b.intraAnimationFrameCounter, 0x05);
  QCOMPARE(b.animFrameCounter, 0x06);
  QCOMPARE(b.faceDir, 0x08);
  QCOMPARE(b.walkAnimationCounter, 0x10);
  QCOMPARE(b.yDisp, 0x08);
  QCOMPARE(b.xDisp, 0x08);
  QCOMPARE(b.mapY, 0x09);
  QCOMPARE(b.mapX, 0x07);
  QCOMPARE(b.movementByte, 0xFE);
  QCOMPARE(b.grassPriority, 0x80);
  QCOMPARE(b.imageBaseOffset, 0x44);
  // NPC table
  QCOMPARE(b.getRangeDirByte(), 0x03);
  QCOMPARE(b.getTextID(), 0x21);
  QCOMPARE(b.getTrainerClassOrItemID(), 0x34);
  QCOMPARE(b.getTrainerSetID(), 0x56);
}

void TestSpriteData::missables_saveThenCheck()
{
  SaveFile sf;

  SpriteData* player = new SpriteData(false);   // slot 0, not a missable
  SpriteData* npc    = new SpriteData(true);    // slot 1
  npc->setMissableIndex(7);

  QVector<SpriteData*> sprites{ player, npc };
  SpriteData::saveMissables(&sf, sprites);

  // A fresh sprite checking slot 1 should pick up missable index 7.
  SpriteData probe(true);
  probe.checkMissable(&sf, 1);
  QCOMPARE(probe.getMissableIndex(), 7);

  // Slot 2 has no missable entry -> stays unset/whatever it was (we set it first).
  SpriteData probe2(true);
  probe2.setMissableIndex(99);
  probe2.checkMissable(&sf, 2);
  QCOMPARE(probe2.getMissableIndex(), 99);   // unchanged: no match for slot 2

  delete player; delete npc;
}

// ─────────────────────────────────────────────────────────────────────────────────────────
// The 2026-07-13 corrections. Each of these fails, by name and with the byte, if the bug is
// put back -- which was checked, one at a time, before they were committed.
// ─────────────────────────────────────────────────────────────────────────────────────────

/// MOVEMENT BYTE 1: `STAY = $FF`, `WALK = $FE`. The enum had these INVERTED, and
/// load(MapDBEntrySprite*) therefore wrote WALK for every STAY sprite in the game.
///
/// Verified on the real cartridge (scripts/emu/probe_sprite_persistence.py): booting Pallet
/// Town, the console's own WRAM holds movement byte 1 = $FF for Oak (STAY) and $FE for the
/// Girl and the Fisher (WALK).
void TestSpriteData::movementByte1_stayIsFF_walkIsFE()
{
  QCOMPARE((int)SpriteMobility::Stay, 0xFF);
  QCOMPARE((int)SpriteMobility::Walk, 0xFE);

  // Now drive it through the real DB path, on the real map the console was booted with.
  MapDBEntry* pallet = nullptr;
  for(int m = 0; m < MapsDB::inst()->getStoreSize(); m++) {
    MapDBEntry* map = MapsDB::inst()->getStoreAt(m);
    if(map != nullptr && map->getName() == "Pallet Town") { pallet = map; break; }
  }
  QVERIFY2(pallet != nullptr, "Pallet Town is missing from maps.json");

  auto mapSprites = pallet->getSprites();
  QVERIFY(mapSprites.size() >= 3);

  for(MapDBEntrySprite* entry : mapSprites) {
    SpriteData s(entry);
    const int expected = (entry->getMove() == "Stay") ? 0xFF : 0xFE;
    QCOMPARE(s.movementByte, expected);
  }
}

/// GRASS PRIORITY: `$80` = IN grass. The enum had it inverted, so reset() flagged every
/// blank sprite as standing in tall grass.
void TestSpriteData::grassPriority_80MeansInGrass()
{
  QCOMPARE((int)SpriteGrass::InGrass, 0x80);
  QCOMPARE((int)SpriteGrass::NotInGrass, 0x00);

  SpriteData s;                       // a fresh sprite
  QCOMPARE(s.grassPriority, 0x00);    // is standing on open ground
}

/// MOVEMENT BYTE 2: maps.json's `face` (a string, on STAY sprites) and `range` (a number, on
/// WALK sprites) are TWO CURATIONS OF ONE BYTE. Whichever the data carries, it must land in
/// rangeDirByte -- the byte the game reads out of wMapSpriteData.
///
/// The cartridge agrees: Pallet's Oak (`"face": "None"`) reads movement byte 2 = $FF; the
/// Girl (`"range": 0`) reads $00.
void TestSpriteData::movement2_faceAndRangeAreTheSameByte()
{
  int checked = 0;

  for(int m = 0; m < MapsDB::inst()->getStoreSize(); m++) {
    MapDBEntry* map = MapsDB::inst()->getStoreAt(m);
    if(map == nullptr) continue;

    for(MapDBEntrySprite* entry : map->getSprites()) {
      SpriteData s(entry);

      int expected;
      if(entry->getRange() >= 0)                              expected = entry->getRange();
      else if(entry->getFace() == "None")                     expected = 0xFF;
      else if(entry->getFace() == "Down")                     expected = 0xD0;
      else if(entry->getFace() == "Up")                       expected = 0xD1;
      else if(entry->getFace() == "Left")                     expected = 0xD2;
      else if(entry->getFace() == "Right")                    expected = 0xD3;
      else if(entry->getFace() == "Boulder Movement Byte 2")  expected = 0x10;
      else                                                    expected = 0x00;   // ANY_DIR

      QCOMPARE(s.getRangeDirByte(), expected);
      checked++;
    }
  }

  QVERIFY2(checked > 900, "expected to walk every map sprite in the game");
}

/// ...and movement byte 2 must NEVER end up in faceDir. That field is `spritestatedata1`
/// field 9 -- the ANIMATION facing -- and its only legal values are $0/$4/$8/$C. The old code
/// wrote $FF (NONE) and $D0-$D3 straight into it.
void TestSpriteData::movement2_neverLandsInTheFacingField()
{
  for(int m = 0; m < MapsDB::inst()->getStoreSize(); m++) {
    MapDBEntry* map = MapsDB::inst()->getStoreAt(m);
    if(map == nullptr) continue;

    for(MapDBEntrySprite* entry : map->getSprites()) {
      SpriteData s(entry);
      QVERIFY2(s.faceDir == 0x0 || s.faceDir == 0x4 || s.faceDir == 0x8 || s.faceDir == 0xC,
               qPrintable(QString("faceDir = 0x%1 on %2 -- that is a movement-byte-2 value, "
                                  "not a facing")
                            .arg(s.faceDir, 2, 16, QChar('0'))
                            .arg(map->getName())));
    }
  }
}

/// Every field the game actually keeps in the two sprite-state tables must survive a
/// save->load round trip -- including the five we never read until 2026-07-13 (Y-adjusted,
/// X-adjusted, collision data, original facing, and the duplicate picture id).
void TestSpriteData::spriteStateTables_roundTripEveryFieldTheGameKeeps()
{
  SaveFile sf;
  const var8 index = 5;

  SpriteData a(true);
  a.pictureID = 0x2E;  a.movementStatus = 1;   a.imageIndex = 0xFF;
  a.yStepVector = 0xFF; a.yPixels = 0x3C;      a.xStepVector = 0x01; a.xPixels = 0x40;
  a.intraAnimationFrameCounter = 3;            a.animFrameCounter = 2;
  a.faceDir = 0x0C;
  a.yAdjusted = 0x11;  a.xAdjusted = 0x22;     a.collisionData = 0x33;   // fields a, b, c
  a.walkAnimationCounter = 0x10;
  a.yDisp = 0x08;      a.xDisp = 0x08;
  a.mapY = 0x0D;       a.mapX = 0x0D;
  a.movementByte = 0xFF;                        // STAY
  a.grassPriority = 0x80;                       // in grass
  a.movementDelay = 0x07;
  a.origFacingDir = 0x04;                       // field 9
  a.pictureIDCopy = 0x2E;                       // field d
  a.imageBaseOffset = 0x0B;                     // field e

  a.save(&sf, index);

  SpriteData b(false, &sf, index);
  QCOMPARE(b.yAdjusted, 0x11);
  QCOMPARE(b.xAdjusted, 0x22);
  QCOMPARE(b.collisionData, 0x33);
  QCOMPARE(b.origFacingDir, 0x04);
  QCOMPARE(b.pictureIDCopy, 0x2E);
  QCOMPARE(b.imageBaseOffset, 0x0B);
  QCOMPARE(b.movementDelay, 0x07);
  QCOMPARE(b.movementByte, 0xFF);
  QCOMPARE(b.grassPriority, 0x80);
  QCOMPARE(b.faceDir, 0x0C);
  QCOMPARE(b.mapX, 0x0D);
  QCOMPARE(b.mapY, 0x0D);

  // And the bytes the game does NOT use (StateData1 d/e/f, StateData2 1/a/b/c/f) must still
  // be untouched -- a blank SaveFile is all zeroes, so writing any of them would show up.
  auto ts = sf.toolset;
  for(var16 off : { 0x2D2C + 0x10 * index + 0xD, 0x2D2C + 0x10 * index + 0xE,
                    0x2D2C + 0x10 * index + 0xF, 0x2E2C + 0x10 * index + 0x1,
                    0x2E2C + 0x10 * index + 0xA, 0x2E2C + 0x10 * index + 0xB,
                    0x2E2C + 0x10 * index + 0xC, 0x2E2C + 0x10 * index + 0xF })
    QCOMPARE((int)ts->getByte(off), 0);
}

void TestSpriteData::mapSprites_allLinksResolve()
{
  // Invariant guard: after deepLink, every map sprite resolves its picture.
  int maps = MapsDB::inst()->getStoreSize();
  int total = 0, nulls = 0;
  for(int m = 0; m < maps; m++) {
    MapDBEntry* map = MapsDB::inst()->getStoreAt(m);
    if(map == nullptr) continue;
    for(MapDBEntrySprite* s : map->getSprites()) {
      total++;
      if(s == nullptr || s->getToSprite() == nullptr) nulls++;
    }
  }
  QVERIFY(total > 0);
  QCOMPARE(nulls, 0);
}

void TestSpriteData::setToAllAndRandomizeAll_overEveryMap()
{
  // Drives load(MapDBEntrySprite*) (all face/move/missable/range branches + the
  // TRAINER/ITEM/POKEMON type branches incl. the "0"-named item guard), setTo,
  // setToAll, randomize, and randomizeAll (incl. the boulder-skip) over real data.
  // If any type resolver were unguarded-null this would crash the exe.
  int maps = MapsDB::inst()->getStoreSize();
  int built = 0;
  for(int m = 0; m < maps; m++) {
    MapDBEntry* map = MapsDB::inst()->getStoreAt(m);
    if(map == nullptr) continue;
    auto mapSprites = map->getSprites();

    QVector<SpriteData*> a = SpriteData::setToAll(mapSprites);
    for(SpriteData* s : a) { QVERIFY(s != nullptr); delete s; }

    QVector<SpriteData*> b = SpriteData::randomizeAll(mapSprites);
    built += b.size();
    for(SpriteData* s : b) { QVERIFY(s != nullptr); delete s; }
  }
  // randomizeAll prepends a player sprite per map, so it builds more than 0.
  QVERIFY(built > maps);
}

void TestSpriteData::setTo_nullIsSafeReset()
{
  // setTo(nullptr) must reset to a blank sprite, not crash.
  SpriteData s;
  s.pictureID = 55;
  s.setTo(nullptr);
  QCOMPARE(s.pictureID, 0);          // reset() ran
  QCOMPARE(s.mapX, 4);
}

QTEST_GUILESS_MAIN(TestSpriteData)
#include "tst_sprite_data.moc"
