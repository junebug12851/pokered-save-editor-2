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
 * @file tst_db_stores.cpp
 * @brief Store-accessor sweep across every sub-DB that exposes getStoreAt(): a
 *        valid index (0) resolves to a real entry, and an out-of-range index
 *        returns nullptr instead of crashing. This is the guard that caught the
 *        inverted bounds check in CreditsDB::getStoreAt() (fixed alongside).
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/creditsdb.h>
#include <pse-db/eventsdb.h>
#include <pse-db/flydb.h>
#include <pse-db/fontsdb.h>
#include <pse-db/gamecornerdb.h>
#include <pse-db/itemsdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/missablesdb.h>
#include <pse-db/moves.h>
#include <pse-db/music.h>
#include <pse-db/pokemon.h>
#include <pse-db/scripts.h>
#include <pse-db/sprites.h>
#include <pse-db/spriteset.h>
#include <pse-db/tileset.h>
#include <pse-db/trades.h>
#include <pse-db/trainers.h>
#include <pse-db/types.h>
#include <pse-db/eventpokemondb.h>

// Verify a DB's store accessor: non-empty stores resolve index 0, and an
// out-of-range index returns nullptr (no crash).
#define CHECK_STORE(DBCLASS) do {                                              \
    auto* _db = DBCLASS::inst();                                               \
    QVERIFY2(_db != nullptr, #DBCLASS "::inst() null");                        \
    const int _n = _db->getStoreSize();                                       \
    QVERIFY2(_n >= 0, #DBCLASS " negative store size");                        \
    if(_n > 0)                                                                 \
      QVERIFY2(_db->getStoreAt(0) != nullptr, #DBCLASS " getStoreAt(0) null"); \
    QVERIFY2(_db->getStoreAt(_n + 100) == nullptr, #DBCLASS " oob not null");  \
    QVERIFY2(_db->getStoreAt(-1) == nullptr, #DBCLASS " neg index not null");  \
  } while(0)

class TestDbStores : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void allStores_resolveValidAndRejectOutOfRange();
};

void TestDbStores::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

void TestDbStores::allStores_resolveValidAndRejectOutOfRange()
{
  CHECK_STORE(CreditsDB);     // regression guard for the inverted getStoreAt bounds
  CHECK_STORE(EventsDB);
  CHECK_STORE(FlyDB);
  CHECK_STORE(FontsDB);
  CHECK_STORE(GameCornerDB);
  CHECK_STORE(ItemsDB);
  CHECK_STORE(MapsDB);
  CHECK_STORE(MissablesDB);
  CHECK_STORE(MovesDB);
  CHECK_STORE(MusicDB);
  CHECK_STORE(PokemonDB);
  CHECK_STORE(ScriptsDB);
  CHECK_STORE(SpritesDB);
  CHECK_STORE(SpriteSetDB);
  CHECK_STORE(TilesetDB);
  CHECK_STORE(TradesDB);
  CHECK_STORE(TrainersDB);
  CHECK_STORE(TypesDB);
  CHECK_STORE(EventPokemonDB);
}

QTEST_GUILESS_MAIN(TestDbStores)
#include "tst_db_stores.moc"
