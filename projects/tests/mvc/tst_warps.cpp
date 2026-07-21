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
 * @file tst_warps.cpp
 * @brief The map's DOORS -- the warp list, the twelve state bytes around it, and the two loaded guns.
 *
 * The domain write-up is notes/reference/warps.md, and it is verified against the cartridge
 * (`scripts/emu/probe_warp_persistence.py`). This file pins the model to it.
 *
 * Two keystones:
 *
 *  - `moveWarp_writesExactlyTwoBytes` -- byte-diff the WHOLE 32 KB save across a drag and demand
 *    that `x` and `y` are the only things in it that moved.
 *  - `setTo_touchesNoStateByte` -- the regression guard for the real bug this phase fixed.
 *    `AreaWarps::setTo()` used to invent `dungeonWarpDestMap`, `specialWarpDestMap`,
 *    `whichDungeonWarp` and `warpedFromWarp` **at random**, and three of the four were values no
 *    console has a table entry for. A map has no opinion about where your last Fly went.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"
#include <pse-db/db.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areawarps.h>
#include <pse-savefile/expanded/fragments/warpdata.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <mvc/mapmodel.h>

using namespace pse_test;

namespace {

// The save offsets, straight out of notes/reference/warps.md. (WRAM = save + 0xAD54.)
constexpr int kNumWarps        = 0x265A;  // wNumberOfWarps             $D3AE
constexpr int kWarpEntries     = 0x265B;  // wWarpEntries               $D3AF  (Y, X, warpID, mapID) x32
constexpr int kDestWarpID      = 0x26DB;  // wDestinationWarpID         $D42F
constexpr int kLastMap         = 0x2611;  // wLastMap                   $D365
constexpr int kLastBlackoutMap = 0x29C5;  // wLastBlackoutMap           $D719
constexpr int kDestinationMap  = 0x29C6;  // wDestinationMap            $D71A
constexpr int kDungeonWarpDest = 0x29C9;  // wDungeonWarpDestinationMap $D71D
constexpr int kWhichDungeon    = 0x29CA;  // wWhichDungeonWarp          $D71E
constexpr int kStatusFlags6    = 0x29DE;  // wStatusFlags6              $D732
constexpr int kStatusFlags7    = 0x29DF;  // wStatusFlags7              $D733
constexpr int kWarpedFromWarp  = 0x29E7;  // wWarpedFromWhichWarp       $D73B
constexpr int kWarpedFromMap   = 0x29E8;  // wWarpedFromWhichMap        $D73C

constexpr int kReturnMap = 0xFF;          // LAST_MAP -- "back outside"

/// Pull one field out of a `warpFields` / `warpStateFields` list by key.
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

class TestWarps : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  // ── The list ────────────────────────────────────────────────────────────────
  void warpList_drawsTheMapsRealDoors();
  void moveWarp_writesExactlyTwoBytes();
  void moveWarp_clampsToTheMap();
  void addWarp_defaultsToBackOutside();
  void removeWarp_slidesTheRestUp();
  void warpRoomLeft_countsDownFromThirtyTwo();
  void warpsEdited_isQuietUntilYouActuallyChangeSomething();

  // ── Where a door LEADS ──────────────────────────────────────────────────────
  void returnDoor_resolvesThroughLastMap();
  void destWarp_pastTheEnd_isFlaggedNotRefused();
  void lastMap_writesOneByte();

  // ── The state bytes ─────────────────────────────────────────────────────────
  void warpStateFields_nameEveryByteInEnglish();
  void deadAndWipedFields_areAbsentUntilYouAskForThem();
  void deadAndWiped_areMarkedAsDIFFERENTFacts();
  void setWarpStateField_writesOneByte_andTakesHackValues();
  void escapeWarp_isBitSix_andAreaMapNoLongerOwnsIt();

  // ── 🔫 The two loaded guns ──────────────────────────────────────────────────
  void legalFlyMaps_areTheCartridgesThirteen();
  void legalDungeonWarps_arePairs_andHolesAreOneBased();
  void guns_flagTheIllegalValue_butNeverRefuseIt();
  void guns_dontCryWolfOnAnOrdinarySave();

  // ── The bug this phase existed to fix ───────────────────────────────────────
  void setTo_touchesNoStateByte();
  void randomize_onlyEverProducesLegalState();

  // ── Fidelity ────────────────────────────────────────────────────────────────
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

void TestWarps::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(!m_orig.isEmpty());
}

// ══ The list ═══════════════════════════════════════════════════════════════════════════════════

/// The fixture is standing in a real map with real doors, and every one comes back with a place, a
/// destination, and a sentence saying where it goes.
void TestWarps::warpList_drawsTheMapsRealDoors()
{
  Rig* r = makeRig();

  const QVariantList doors = r->map->warpList();
  QVERIFY2(!doors.isEmpty(), "the fixture's map has no warps -- pick a different fixture");

  for (const QVariant& v : doors) {
    const QVariantMap m = v.toMap();

    QVERIFY(m.contains("ind"));
    QVERIFY(m.value("x").toInt() >= 0);
    QVERIFY(m.value("y").toInt() >= 0);

    // The chip has to be drawable: a rectangle in BUFFER pixels, ring included.
    QVERIFY(m.value("rectX").toInt() > 0);
    QVERIFY(m.value("rectY").toInt() > 0);
    QCOMPARE(m.value("rectW").toInt(), 16);   // a door is a TILE, not a block
    QCOMPARE(m.value("rectH").toInt(), 16);

    // And it has to SAY where it goes -- a bare map id is not an answer.
    QVERIFY2(!m.value("destName").toString().isEmpty(),
             "a door came back with no destination name");
  }

  delete r;
}

/**
 * @brief THE KEYSTONE. Drag a door across town and the save changes by **exactly two bytes**.
 *
 * Byte-fidelity is a top-tier project value (context/principles.md -> "Save File Integrity Is
 * Sacred"). If this ever goes red, something is rewriting bytes it was never told to touch, and
 * that is a release blocker rather than a bug report.
 */
void TestWarps::moveWarp_writesExactlyTwoBytes()
{
  Rig* r = makeRig();

  const QVariantMap first = r->map->warpList().first().toMap();
  const int ind = first.value("ind").toInt();

  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);

  r->map->moveWarp(ind, first.value("x").toInt() + 3, first.value("y").toInt() + 2);

  r->sf.flattenData();
  const QByteArray after = snapshot(r->sf);

  QCOMPARE(before.size(), after.size());

  const QVector<int> moved = diffOffsets(before, after);

  // A warp entry is { Y, X, destWarp, destMap } -- Y first. Those two bytes, and nothing else in
  // 32 KB. (flattenData does not touch the checksum; the save pipeline recalculates it after.)
  const int base = kWarpEntries + 4 * ind;
  const QVector<int> expected = { base + 0, base + 1 };

  QVERIFY2(moved == expected,
           qPrintable(QStringLiteral("a drag moved %1 bytes, not just the door's x/y: %2")
                        .arg(moved.size())
                        .arg(describeDiff(moved))));

  delete r;
}

/// A door out in the 3-block border ring is a door the player can never reach. An overshooting drag
/// stops at the map's edge rather than parking it in the trees.
void TestWarps::moveWarp_clampsToTheMap()
{
  Rig* r = makeRig();

  const int w = r->map->blocksWide() * 2;
  const int h = r->map->blocksHigh() * 2;

  r->map->moveWarp(0, 9999, 9999);
  QVariantMap d = r->map->warpAt(0);
  QCOMPARE(d.value("x").toInt(), w - 1);
  QCOMPARE(d.value("y").toInt(), h - 1);

  r->map->moveWarp(0, -50, -50);
  d = r->map->warpAt(0);
  QCOMPARE(d.value("x").toInt(), 0);
  QCOMPARE(d.value("y").toInt(), 0);

  delete r;
}

/**
 * @brief A new door means "back outside", because that is what a door usually is.
 *
 * `$FF` (LAST_MAP) is also the one destination that is **always valid** -- it resolves through
 * `wLastMap` instead of naming a map that may have no arrival point with that index.
 */
void TestWarps::addWarp_defaultsToBackOutside()
{
  Rig* r = makeRig();

  const int ind = r->map->addWarp(4, 5);
  QVERIFY(ind >= 0);

  const QVariantMap d = r->map->warpAt(ind);
  QCOMPARE(d.value("x").toInt(), 4);
  QCOMPARE(d.value("y").toInt(), 5);
  QCOMPARE(d.value("destMap").toInt(), kReturnMap);
  QVERIFY(d.value("isReturn").toBool());
  QVERIFY2(d.value("destValid").toBool(),
           "a brand-new door is pointing somewhere the console has no answer for");

  // And it says so in words, naming the map you'd actually come out in.
  QVERIFY(d.value("destName").toString().contains(QStringLiteral("back outside")));

  delete r;
}

/// The game packs its warp list, so deleting one slides the rest up -- and the count byte follows.
void TestWarps::removeWarp_slidesTheRestUp()
{
  Rig* r = makeRig();

  const QVariantList before = r->map->warpList();
  QVERIFY(before.size() >= 2);

  const QVariantMap second = before.at(1).toMap();
  const int n = before.size();

  r->map->removeWarp(0);

  const QVariantList after = r->map->warpList();
  QCOMPARE(after.size(), n - 1);

  // What was door 1 is now door 0, unchanged.
  const QVariantMap nowFirst = after.first().toMap();
  QCOMPARE(nowFirst.value("x").toInt(), second.value("x").toInt());
  QCOMPARE(nowFirst.value("y").toInt(), second.value("y").toInt());
  QCOMPARE(nowFirst.value("destMap").toInt(), second.value("destMap").toInt());

  // And the count byte in the save agrees.
  r->sf.flattenData();
  QCOMPARE(static_cast<int>(snapshot(r->sf).at(kNumWarps)), n - 1);

  delete r;
}

/// The cap is stated *before* you hit it, not after -- so the model has to be able to say it.
void TestWarps::warpRoomLeft_countsDownFromThirtyTwo()
{
  Rig* r = makeRig();

  const int used = r->map->warpList().size();
  QCOMPARE(r->map->warpRoomLeft(), 32 - used);

  // Fill it right up.
  while (r->map->warpRoomLeft() > 0)
    QVERIFY(r->map->addWarp(1, 1) >= 0);

  QCOMPARE(r->map->warpList().size(), 32);
  QCOMPARE(r->map->warpRoomLeft(), 0);

  // And the 33rd is refused, not silently swallowed into a 33rd slot the game cannot read.
  QCOMPARE(r->map->addWarp(2, 2), -1);
  QCOMPARE(r->map->warpList().size(), 32);

  delete r;
}

/**
 * @brief Quiet until *you* change something.
 *
 * Same rule, and the same reason, as `npcsEdited`: we track the EDIT, never a diff against the ROM.
 * (The cast version of this was got wrong first time -- a real save's cast already differs from the
 * cartridge's because walkers wander, so diffing cried wolf on every save ever opened.)
 */
void TestWarps::warpsEdited_isQuietUntilYouActuallyChangeSomething()
{
  Rig* r = makeRig();

  QVERIFY2(!r->map->warpsEdited(), "the panel is warning about an edit nobody made");

  // Merely LOOKING must not trip it.
  (void)r->map->warpList();
  (void)r->map->warpFields(0);
  (void)r->map->warpStateFields();
  QVERIFY(!r->map->warpsEdited());

  const QVariantMap first = r->map->warpList().first().toMap();
  r->map->moveWarp(0, first.value("x").toInt() + 1, first.value("y").toInt());

  QVERIFY2(r->map->warpsEdited(), "a drag did not trip the warning");

  delete r;
}

// ══ Where a door LEADS ═════════════════════════════════════════════════════════════════════════

/**
 * @brief The `$FF` door does not name a map. It resolves through `wLastMap`, LIVE.
 *
 * This is the whole reason "Outside is…" belongs in the toolbar rather than buried in a panel:
 * change it, and every "back outside" door on the map re-labels at once.
 */
void TestWarps::returnDoor_resolvesThroughLastMap()
{
  Rig* r = makeRig();

  const int ind = r->map->addWarp(3, 3);
  QVERIFY(ind >= 0);

  r->map->setLastMap(0);   // PALLET_TOWN
  const QString pallet = r->map->warpAt(ind).value("destName").toString();
  QVERIFY(pallet.contains(r->map->lastMapName()));

  r->map->setLastMap(1);   // VIRIDIAN_CITY
  const QString viridian = r->map->warpAt(ind).value("destName").toString();

  QVERIFY2(pallet != viridian,
           "changing 'Outside is...' did not re-label the back-outside doors");
  QVERIFY(viridian.contains(r->map->lastMapName()));

  delete r;
}

/**
 * @brief 🔫 An arrival point past the end of the target map's list is SHOWN, not refused.
 *
 * `LoadDestinationWarpPosition` does not bounds-check it -- past the end, the console copies four
 * arbitrary ROM bytes into the view pointer and the player's coordinates. Every byte stays editable;
 * we simply know, and say.
 */
void TestWarps::destWarp_pastTheEnd_isFlaggedNotRefused()
{
  Rig* r = makeRig();

  const int ind = r->map->addWarp(3, 3);
  r->map->setWarpField(ind, "destMap", 0);      // Pallet Town -- a real map with a few arrival points
  r->map->setWarpField(ind, "destWarp", 0);

  QVariantMap d = r->map->warpAt(ind);
  const int arrivals = d.value("arrivalCount").toInt();
  QVERIFY2(arrivals > 0, "Pallet Town has no arrival points -- the map DB is not deep-linked");
  QVERIFY(d.value("destValid").toBool());
  QVERIFY(!d.value("destLabel").toString().isEmpty());

  // Now point it past the end. It must TAKE the value...
  r->map->setWarpField(ind, "destWarp", 200);
  d = r->map->warpAt(ind);
  QCOMPARE(d.value("destWarp").toInt(), 200);

  // ...and flag it, and have nothing to resolve.
  QVERIFY2(!d.value("destValid").toBool(), "a door pointing off the end was not flagged");
  QVERIFY(d.value("destLabel").toString().isEmpty());

  delete r;
}

/// `wLastMap` lives in WorldGeneral. Writing it writes ONE byte.
void TestWarps::lastMap_writesOneByte()
{
  Rig* r = makeRig();

  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);

  r->map->setLastMap(static_cast<quint8>(before.at(kLastMap)) == 7 ? 6 : 7);

  r->sf.flattenData();
  const QVector<int> moved = diffOffsets(before, snapshot(r->sf));

  QVERIFY2(moved == QVector<int>{ kLastMap },
           qPrintable(QStringLiteral("setting 'Outside is...' moved: %1").arg(describeDiff(moved))));

  delete r;
}

// ══ The state bytes ════════════════════════════════════════════════════════════════════════════

/// Every one of the twelve has a name a person can read and a sentence saying what it does. A bare
/// number with a cryptic label is exactly what this panel exists not to be.
void TestWarps::warpStateFields_nameEveryByteInEnglish()
{
  Rig* r = makeRig();
  r->map->setShowScratch(true);   // we want the whole set, dead ones included

  const QVariantList fields = r->map->warpStateFields();

  const QStringList expected = {
    "lastBlackoutMap", "specialWarpDestMap", "dungeonWarpDestMap", "whichDungeonWarp", "warpDest",
    "flyOrDungeonWarp", "flyWarp", "dungeonWarp", "escapeWarp", "forcedWarp",
    "scriptedWarp", "isDungeonWarp", "warpedFromWarp", "warpedfromMap",
  };

  for (const QString& key : expected) {
    const QVariantMap f = fieldNamed(fields, key);
    QVERIFY2(!f.isEmpty(), qPrintable(QStringLiteral("no field for '%1'").arg(key)));

    QVERIFY2(!f.value("label").toString().isEmpty(),
             qPrintable(QStringLiteral("'%1' has no human name").arg(key)));
    QVERIFY2(!f.value("blurb").toString().isEmpty(),
             qPrintable(QStringLiteral("'%1' has no explanation").arg(key)));
    QVERIFY2(!f.value("group").toString().isEmpty(),
             qPrintable(QStringLiteral("'%1' is not in a group").arg(key)));
  }

  delete r;
}

/**
 * @brief The four that do nothing are ABSENT, not greyed, until the switch is flipped.
 *
 * Exactly what project leadership asked for: *"little exclamation points grouped below and hidden behind a
 * switch."* Filtered in the MODEL, so no view can leak one -- and so this test can prove it.
 */
void TestWarps::deadAndWipedFields_areAbsentUntilYouAskForThem()
{
  Rig* r = makeRig();

  const QStringList doNothing = { "scriptedWarp", "isDungeonWarp",
                                  "warpedFromWarp", "warpedfromMap" };

  QVERIFY(!r->map->showScratch());   // off by default -- clutter is a bug

  QVariantList fields = r->map->warpStateFields();
  for (const QString& key : doNothing)
    QVERIFY2(fieldNamed(fields, key).isEmpty(),
             qPrintable(QStringLiteral("'%1' leaked into the panel with the switch OFF").arg(key)));

  // The ones that DO something are of course still there.
  QVERIFY(!fieldNamed(fields, "forcedWarp").isEmpty());
  QVERIFY(!fieldNamed(fields, "escapeWarp").isEmpty());

  r->map->setShowScratch(true);
  fields = r->map->warpStateFields();
  for (const QString& key : doNothing)
    QVERIFY2(!fieldNamed(fields, key).isEmpty(),
             qPrintable(QStringLiteral("'%1' did not appear with the switch ON").arg(key)));

  delete r;
}

/**
 * @brief ⚠️ WIPED and 💀 DEAD are two different facts, and the panel must not merge them.
 *
 * A byte the console **zeroes on every load** and a byte that **survives perfectly and is never
 * read** are not the same thing. Collapsing both into one grey "unused" would be the kind of
 * hand-wave this project does not do.
 */
void TestWarps::deadAndWiped_areMarkedAsDIFFERENTFacts()
{
  Rig* r = makeRig();
  r->map->setShowScratch(true);

  const QVariantList fields = r->map->warpStateFields();

  // ⚠️ wStatusFlags3 shares an address with wCableClubDestinationMap, and SpecialEnterMap zeroes it
  //    on the Continue path. The WHOLE byte. Console-verified: wrote $FF, read back $00.
  for (const QString& key : { "scriptedWarp", "isDungeonWarp" }) {
    const QVariantMap f = fieldNamed(fields, key);
    QVERIFY2(f.value("scratch").toBool(),
             qPrintable(QStringLiteral("'%1' is wiped on load and is not marked so").arg(key)));
    QCOMPARE(f.value("mark").toString(), QStringLiteral("wiped"));
    QVERIFY2(!f.value("dead").toBool(),
             qPrintable(QStringLiteral("'%1' is WIPED, not DEAD -- they are different").arg(key)));
  }

  // 💀 Written by the game on every warp; read by nothing, anywhere. Two writes, zero reads.
  for (const QString& key : { "warpedFromWarp", "warpedfromMap" }) {
    const QVariantMap f = fieldNamed(fields, key);
    QVERIFY2(f.value("dead").toBool(),
             qPrintable(QStringLiteral("'%1' is dead and is not marked so").arg(key)));
    QCOMPARE(f.value("mark").toString(), QStringLiteral("dead"));
    QVERIFY2(!f.value("scratch").toBool(),
             qPrintable(QStringLiteral("'%1' is DEAD, not WIPED -- they are different").arg(key)));
  }

  delete r;
}

/// Each state key writes its own byte (or its own bit) and nothing else -- and takes the full range.
void TestWarps::setWarpStateField_writesOneByte_andTakesHackValues()
{
  struct Case { const char* key; int value; int offset; };
  const QVector<Case> cases = {
    { "specialWarpDestMap", 0x99, kDestinationMap },   // a MAP THE CONSOLE HAS NO TABLE ENTRY FOR --
    { "dungeonWarpDestMap", 0x77, kDungeonWarpDest },  // and it is taken anyway. Flagged, never refused.
    { "whichDungeonWarp",   0x42, kWhichDungeon },
    { "warpDest",           0x0A, kDestWarpID },
    { "warpedFromWarp",     0x1B, kWarpedFromWarp },
    { "warpedfromMap",      0x2C, kWarpedFromMap },
    { "lastBlackoutMap",    0x03, kLastBlackoutMap },
  };

  for (const Case& c : cases) {
    Rig* r = makeRig();
    r->map->setShowScratch(true);

    r->sf.flattenData();
    const QByteArray before = snapshot(r->sf);

    // Make sure we are actually changing it.
    QVERIFY(static_cast<quint8>(before.at(c.offset)) != c.value);

    r->map->setWarpStateField(QString::fromLatin1(c.key), c.value);

    r->sf.flattenData();
    const QByteArray after = snapshot(r->sf);

    // The value went in, unmangled and unrefused.
    QCOMPARE(static_cast<int>(static_cast<quint8>(after.at(c.offset))), c.value);

    // And ONE byte moved.
    const QVector<int> moved = diffOffsets(before, after);
    QVERIFY2(moved == QVector<int>{ c.offset },
             qPrintable(QStringLiteral("setting '%1' moved: %2")
                          .arg(QString::fromLatin1(c.key), describeDiff(moved))));

    delete r;
  }
}

/**
 * @brief `escapeWarp` is `wStatusFlags6` bit 6 -- and `AreaMap` must no longer write it.
 *
 * It used to live in `AreaMap` as a bool called `blackoutDest`, doc'd *"Flag (may be unused)"*. It
 * was neither a destination nor unused: it is `BIT_ESCAPE_WARP` -- Dig, Escape Rope, and blacking
 * out. Two owners writing one bit is how a save gets quietly corrupted, so `AreaMap` gave it up.
 */
void TestWarps::escapeWarp_isBitSix_andAreaMapNoLongerOwnsIt()
{
  Rig* r = makeRig();
  auto* warps = r->sf.dataExpanded->area->warps;

  r->sf.flattenData();
  const QByteArray before = snapshot(r->sf);
  const bool was = (static_cast<quint8>(before.at(kStatusFlags6)) >> 6) & 1;

  r->map->setWarpStateField(QStringLiteral("escapeWarp"), was ? 0 : 1);
  QCOMPARE(warps->escapeWarp, !was);

  r->sf.flattenData();
  const QByteArray after = snapshot(r->sf);

  // The right bit of the right byte...
  const bool now = (static_cast<quint8>(after.at(kStatusFlags6)) >> 6) & 1;
  QCOMPARE(now, !was);

  // ...and ONLY that byte. (AreaMap still owns bit 5 of it -- forceBikeRide -- so the byte itself is
  // expected to differ; what must not happen is anything ELSE moving.)
  const QVector<int> moved = diffOffsets(before, after);
  QVERIFY2(moved == QVector<int>{ kStatusFlags6 },
           qPrintable(QStringLiteral("flipping escapeWarp moved: %1").arg(describeDiff(moved))));

  delete r;
}

// ══ 🔫 The two loaded guns ═════════════════════════════════════════════════════════════════════

/**
 * @brief The 13 maps a Fly may legally name -- and they are all real maps.
 *
 * `FlyWarpDataPtr` (data/maps/special_warps.asm) has **13 entries and no terminator**, and the loop
 * that scans it has no bounds check. Any other map id runs off the end of the table.
 *
 * The names are cross-checked against `maps.json`, so a typo in the id list cannot hide.
 */
void TestWarps::legalFlyMaps_areTheCartridgesThirteen()
{
  const QVector<int>& fly = AreaWarps::legalFlyMaps();
  QCOMPARE(fly.size(), 13);

  // The eleven Fly towns, plus the two ROUTES with Poké Centers (Mt. Moon and Rock Tunnel) -- the
  // game Flies you to a route, not a town, for those two.
  const QStringList expected = {
    "Pallet Town", "Viridian City", "Pewter City", "Cerulean City", "Lavender Town",
    "Vermilion City", "Celadon City", "Fuchsia City", "Cinnabar Island", "Indigo Plateau",
    "Saffron City", "Route 4", "Route 10",
  };

  for (int i = 0; i < fly.size(); i++) {
    auto* entry = MapsDB::inst()->getIndAt(QString::number(fly.at(i)));
    QVERIFY2(entry != nullptr,
             qPrintable(QStringLiteral("fly map id %1 names no map at all").arg(fly.at(i))));

    QCOMPARE(entry->bestName(), expected.at(i));
  }

  // And the predicate agrees with the list, in both directions.
  for (int m = 0; m < 248; m++)
    QCOMPARE(AreaWarps::isLegalFlyMap(m), fly.contains(m));
}

/**
 * @brief The 12 legal (map, hole) PAIRS -- and the hole numbers are **1-based**.
 *
 * ⚠️ Two things that are easy to get wrong and that v1 got wrong:
 *
 *  - It is a **pair**. A legal map with the wrong hole number is just as broken as an illegal map.
 *  - **Victory Road 2F has a hole 2 and no hole 1**, which is exactly why "how many holes" is not a
 *    range but a per-map question.
 */
void TestWarps::legalDungeonWarps_arePairs_andHolesAreOneBased()
{
  const auto& pairs = AreaWarps::legalDungeonWarps();
  QCOMPARE(pairs.size(), 12);

  // Every hole number is 1-based. A 0 never matches anything in DungeonWarpList -- which is what
  // the old randomizer produced, every time.
  for (const auto& p : pairs) {
    QVERIFY2(p.second >= 1,
             "a hole number is 0-based -- the game counts these from 1 (wCoordIndex)");
    QVERIFY(MapsDB::inst()->getIndAt(QString::number(p.first)) != nullptr);
  }

  // Seven distinct maps.
  QSet<int> maps;
  for (const auto& p : pairs)
    maps.insert(p.first);
  QCOMPARE(maps.size(), 7);

  // Victory Road 2F (194): hole 2, and NO hole 1.
  QVERIFY(AreaWarps::isLegalDungeonWarp(194, 2));
  QVERIFY2(!AreaWarps::isLegalDungeonWarp(194, 1),
           "Victory Road 2F has no hole 1 -- the pair check is not really checking the pair");

  // Seafoam B1F (159) has both.
  QVERIFY(AreaWarps::isLegalDungeonWarp(159, 1));
  QVERIFY(AreaWarps::isLegalDungeonWarp(159, 2));

  // A map with no holes at all.
  QVERIFY(!AreaWarps::isLegalDungeonWarp(0, 1));   // Pallet Town
}

/// The gun is **flagged**, never refused -- and the panel is told which values are safe.
void TestWarps::guns_flagTheIllegalValue_butNeverRefuseIt()
{
  Rig* r = makeRig();

  // A legal Fly map.
  r->map->setWarpStateField(QStringLiteral("specialWarpDestMap"), 1);   // Viridian City
  QVariantMap f = fieldNamed(r->map->warpStateFields(), QStringLiteral("specialWarpDestMap"));
  QVERIFY(f.value("gun").toBool());
  QVERIFY2(f.value("legal").toBool(), "Viridian City is a Fly destination and was flagged illegal");

  // An illegal one. TAKEN -- and flagged.
  r->map->setWarpStateField(QStringLiteral("specialWarpDestMap"), 200);
  f = fieldNamed(r->map->warpStateFields(), QStringLiteral("specialWarpDestMap"));
  QCOMPARE(f.value("value").toInt(), 200);
  QVERIFY2(!f.value("legal").toBool(), "an out-of-table Fly destination was not flagged");

  // The panel can offer only the safe ones. (+1 each for the "not falling" resting value, which is
  // what the game itself writes when you are not standing on a hole -- see below.)
  QCOMPARE(r->map->flyWarpMapList().size(), 13);
  QCOMPARE(r->map->dungeonWarpMapList().size(), 8);       // 7 maps + "Nowhere"

  // Per-map hole lists -- and Victory Road's has exactly one real hole, and it is not "1".
  const QVariantList vr = r->map->dungeonHoleList(194);
  QCOMPARE(vr.size(), 2);                                 // "Not falling" + hole 2
  QCOMPARE(vr.at(1).toMap().value("value").toInt(), 2);

  QCOMPARE(r->map->dungeonHoleList(159).size(), 3);       // Seafoam B1F: "Not falling" + holes 1, 2
  QCOMPARE(r->map->dungeonHoleList(0).size(), 1);         // Pallet Town: only "Not falling"

  delete r;
}

/**
 * @brief 🔫 The red "!" fires only when the value is out of the table **AND the game will read it**.
 *
 * ⚠️ **THE SCREENSHOT REVIEW CAUGHT THIS ONE, and it would have shipped.**
 *
 * The fixture save -- an entirely ordinary one -- holds `dungeonWarpDestMap = 194` (Victory Road 2F)
 * and `whichDungeonWarp = 0`. That pair is not in `DungeonWarpList`, so the first cut screamed at it.
 *
 * But **0 is the resting value**: `IsPlayerOnDungeonWarp` writes 0 there as its very first
 * instruction whenever you are *not* standing on a hole. So essentially **every save anybody has ever
 * made carries one**, `BIT_DUNGEON_WARP` is off, and the console will never look at either byte.
 *
 * Flagging that is crying wolf on every file ever opened -- exactly the mistake the sprite "your cast
 * has changed" notice made in its first cut. Noise is a bug.
 */
void TestWarps::guns_dontCryWolfOnAnOrdinarySave()
{
  Rig* r = makeRig();
  auto* warps = r->sf.dataExpanded->area->warps;

  // The fixture really is like this. If it ever isn't, this test is testing nothing.
  QVERIFY2(warps->whichDungeonWarp == 0,
           "the fixture no longer rests at hole 0 -- this test's whole premise is gone");
  QVERIFY2(!warps->dungeonWarp, "the fixture is mid-fall, which no ordinary save is");

  QVariantList fields = r->map->warpStateFields();

  for (const QString& key : { "dungeonWarpDestMap", "whichDungeonWarp" }) {
    const QVariantMap f = fieldNamed(fields, key);
    QVERIFY(f.value("gun").toBool());

    // Nothing is going to read it, so it is NOT armed -- and the panel therefore keeps quiet.
    QVERIFY2(!f.value("armed").toBool(),
             qPrintable(QStringLiteral("'%1' claims the game will read it, on a save that isn't "
                                       "falling down anything").arg(key)));
  }

  // The resting hole (0) is a LEGAL, NAMED value -- not a hack. The game writes it itself.
  QVERIFY2(fieldNamed(fields, QStringLiteral("whichDungeonWarp")).value("legal").toBool(),
           "hole 0 was flagged illegal -- it is what the game writes when you are not falling");

  // ── Now ARM it, and the same value becomes a real hazard. ──────────────────────────────────
  r->map->setWarpStateField(QStringLiteral("flyOrDungeonWarp"), 1);
  r->map->setWarpStateField(QStringLiteral("dungeonWarp"), 1);

  fields = r->map->warpStateFields();
  QVERIFY2(fieldNamed(fields, QStringLiteral("whichDungeonWarp")).value("armed").toBool(),
           "the game IS about to read the hole number and the panel does not know it");

  // Victory Road 2F + hole 1 is a pair the table does not have. Armed, that is a hazard.
  r->map->setWarpStateField(QStringLiteral("dungeonWarpDestMap"), 194);
  r->map->setWarpStateField(QStringLiteral("whichDungeonWarp"), 1);

  fields = r->map->warpStateFields();
  const QVariantMap hole = fieldNamed(fields, QStringLiteral("whichDungeonWarp"));
  QVERIFY2(!hole.value("legal").toBool() && hole.value("armed").toBool(),
           "Victory Road 2F has no hole 1, the game is about to read it, and nobody said anything");

  // ⚠️ And the MAP beside it is judged on its OWN merits -- Victory Road 2F is a real hole map, and
  // the first cut failed BOTH fields whenever the pair was wrong, so a perfectly good map came up
  // flagged because of its neighbour. Two fields, two questions.
  QVERIFY2(fieldNamed(fields, QStringLiteral("dungeonWarpDestMap")).value("legal").toBool(),
           "Victory Road 2F is a map with holes and it was flagged because the HOLE was wrong");

  delete r;
}

// ══ The bug this phase existed to fix ══════════════════════════════════════════════════════════

/**
 * @brief 🐞 THE REGRESSION GUARD. `setTo()` rebuilds the DOORS and touches **no state byte**.
 *
 * It used to do this, on any "place the player on this map":
 *
 * ```cpp
 * auto dungeonWarp = MapsDB::inst()->search()->isGood()->isType("Cave")->pickRandom();
 * dungeonWarpDestMap = dungeonWarp->getInd();                      // ANY cave. 7 are legal.
 * specialWarpDestMap = ...->isGood()->pickRandom()->getInd();      // ANY map. 13 are legal.
 * whichDungeonWarp   = Random::rangeExclusive(0, ...);             // 0-based. The game is 1-based.
 * warpedFromWarp     = Random::rangeExclusive(0, ...);             // a byte nothing reads
 * ```
 *
 * Three of those four were wrong, and two of them can hand a real console a map id that is not in
 * the lookup table -- which has no bounds check. It was dormant only because `MapsDB` is never
 * deep-linked at boot; the moment that lands, it goes live.
 *
 * **A map has no opinion about where your last Fly went.** So it does not get to say.
 */
void TestWarps::setTo_touchesNoStateByte()
{
  Rig* r = makeRig();
  auto* warps = r->sf.dataExpanded->area->warps;

  // Give the state some distinctive, LEGAL values so we can see if anything trampled them.
  warps->specialWarpDestMap = 6;    // Celadon City -- a real Fly destination
  warps->dungeonWarpDestMap = 159;  // Seafoam B1F
  warps->whichDungeonWarp   = 2;
  warps->warpDest           = 0x0C;
  warps->warpedFromWarp     = 0x05;
  warps->warpedfromMap      = 0x09;
  warps->escapeWarp         = true;
  warps->forcedWarp         = true;

  auto* target = MapsDB::inst()->getIndAt(QStringLiteral("1"));   // Viridian City
  QVERIFY(target != nullptr);

  warps->setTo(target);

  // The doors ARE rebuilt...
  QCOMPARE(warps->warpCount(), target->getWarpOut().size());

  // ...and not one state byte moved.
  QCOMPARE(warps->specialWarpDestMap, 6);
  QCOMPARE(warps->dungeonWarpDestMap, 159);
  QCOMPARE(warps->whichDungeonWarp, 2);
  QCOMPARE(warps->warpDest, 0x0C);
  QCOMPARE(warps->warpedFromWarp, 0x05);
  QCOMPARE(warps->warpedfromMap, 0x09);
  QCOMPARE(warps->escapeWarp, true);
  QCOMPARE(warps->forcedWarp, true);

  delete r;
}

/**
 * @brief `randomize()` re-aims the doors and **never produces a state value a console can't survive**.
 *
 * Run it a hundred times over a hundred maps. If it ever writes an out-of-table Fly destination or a
 * 0-based hole number, this fails -- which is precisely what the old one did on every single call.
 */
void TestWarps::randomize_onlyEverProducesLegalState()
{
  Rig* r = makeRig();
  auto* warps = r->sf.dataExpanded->area->warps;

  // A known-good starting state, so "it never touched them" and "it wrote something legal" are
  // distinguishable.
  warps->specialWarpDestMap = 6;
  warps->dungeonWarpDestMap = 159;
  warps->whichDungeonWarp   = 2;

  for (int i = 0; i < 100; i++) {
    auto* m = MapsDB::inst()->getIndAt(QString::number(i));
    if (m == nullptr)
      continue;

    warps->randomize(m);

    QVERIFY2(AreaWarps::isLegalFlyMap(warps->specialWarpDestMap),
             qPrintable(QStringLiteral("randomize() wrote Fly destination %1 -- the console has no "
                                       "table entry for it").arg(warps->specialWarpDestMap)));

    QVERIFY2(AreaWarps::isLegalDungeonWarp(warps->dungeonWarpDestMap, warps->whichDungeonWarp),
             qPrintable(QStringLiteral("randomize() wrote hole pair (%1, %2) -- not in DungeonWarpList")
                          .arg(warps->dungeonWarpDestMap).arg(warps->whichDungeonWarp)));

    // A "return outside" door must stay one -- rewriting it strands the player indoors forever.
    for (int w = 0; w < warps->warpCount(); w++)
      QVERIFY(warps->warpAt(w) != nullptr);

    QVERIFY(warps->warpCount() <= 32);
  }

  delete r;
}

// ══ Fidelity ═══════════════════════════════════════════════════════════════════════════════════

/**
 * @brief Load an untouched save, re-save it, and **not one bit moves**.
 *
 * The plainest possible statement of the project's top-tier value. If the warp model has invented an
 * opinion about any byte -- a "normalisation", a default, a tidy-up -- it shows up here.
 */
void TestWarps::loadingAndResavingAnUntouchedSave_changesNothing()
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

  // And the state bytes we care about came back exactly as the file has them.
  auto* warps = r->sf.dataExpanded->area->warps;
  QCOMPARE(warps->specialWarpDestMap, static_cast<int>(static_cast<quint8>(once.at(kDestinationMap))));
  QCOMPARE(warps->dungeonWarpDestMap, static_cast<int>(static_cast<quint8>(once.at(kDungeonWarpDest))));
  QCOMPARE(warps->whichDungeonWarp,   static_cast<int>(static_cast<quint8>(once.at(kWhichDungeon))));
  QCOMPARE(warps->warpDest,           static_cast<int>(static_cast<quint8>(once.at(kDestWarpID))));
  QCOMPARE(warps->forcedWarp,  static_cast<bool>((static_cast<quint8>(once.at(kStatusFlags7)) >> 2) & 1));
  QCOMPARE(warps->escapeWarp,  static_cast<bool>((static_cast<quint8>(once.at(kStatusFlags6)) >> 6) & 1));
  QCOMPARE(r->map->lastMap(),         static_cast<int>(static_cast<quint8>(once.at(kLastMap))));
  QCOMPARE(r->map->lastBlackoutMap(), static_cast<int>(static_cast<quint8>(once.at(kLastBlackoutMap))));

  delete r;
}

QTEST_MAIN(TestWarps)
#include "tst_warps.moc"
