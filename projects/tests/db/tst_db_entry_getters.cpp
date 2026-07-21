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
 * @file tst_db_entry_getters.cpp
 * @brief Drives the getter-method APIs of the two richest getter-based db entries,
 *        ItemDBEntry and MapDBEntry (most other entries expose public fields). For
 *        each it calls every scalar getter and checks each indexed list's size vs
 *        its At() accessor for consistency -- covering getter code that the
 *        integrity/store sweeps never call.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>

class TestDbEntryGetters : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void itemEntry_gettersResolve();
  void itemStore_everyEntryGetterIsCallable();
  void mapEntry_gettersAndListAccessors();
};

void TestDbEntryGetters::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

void TestDbEntryGetters::itemEntry_gettersResolve()
{
  ItemDBEntry* potion = ItemsDB::inst()->getIndAt(QStringLiteral("POTION"));
  QVERIFY(potion != nullptr);

  QCOMPARE(potion->getName(), QStringLiteral("POTION"));
  QVERIFY(potion->getInd() >= 0);
  QVERIFY(!potion->getReadable().isEmpty());
  QVERIFY(potion->getPrice() >= 0);
  // Booleans + TM/HM numbers: just exercised (POTION is not a TM/HM, not glitch).
  QCOMPARE(potion->getGlitch(), false);
  (void)potion->getOnce();
  (void)potion->getTm();
  (void)potion->getHm();
  (void)potion->getToMove();        // POTION has no move link; may be null
  (void)potion->getToGameCorner();

  // Indexed list: size must agree with the At() accessor over its whole range.
  const int n = potion->getToMapSpriteItemSize();
  QVERIFY(n >= 0);
  for(int i = 0; i < n; i++)
    QVERIFY(potion->getToMapSpriteItemAt(i) != nullptr);
}

void TestDbEntryGetters::itemStore_everyEntryGetterIsCallable()
{
  const QVector<ItemDBEntry*> store = ItemsDB::inst()->getStore();
  QVERIFY(!store.isEmpty());

  for(ItemDBEntry* e : store) {
    QVERIFY(e != nullptr);
    QVERIFY(e->getInd() >= 0);
    if(!e->getGlitch())
      QVERIFY2(!e->getName().isEmpty(), "a non-glitch item has an empty name");
    (void)e->getPrice(); // may be a negative sentinel for unsellable/key items
    QVERIFY(e->getToMapSpriteItemSize() >= 0);
  }
}

void TestDbEntryGetters::mapEntry_gettersAndListAccessors()
{
  MapsDB* db = MapsDB::inst();
  const int maps = db->getStoreSize();
  QVERIFY(maps > 0);

  // Sample the first several maps; for each, exercise the scalar getters and
  // verify every indexed list's size lines up with its At() accessor.
  const int sample = qMin(maps, 12);
  for(int m = 0; m < sample; m++) {
    MapDBEntry* e = db->getStoreAt(m);
    QVERIFY(e != nullptr);

    QVERIFY(e->getInd() >= 0);
    if(!e->getGlitch())
      QVERIFY2(!e->getName().isEmpty(), "a non-glitch map has an empty name");
    (void)e->getSpecial();
    (void)e->getMonRate();      // negative sentinel for maps with no wild encounters
    (void)e->getMonRateWater();
    (void)e->getSpriteSet();

    for(int i = 0; i < e->getWarpOutSize(); i++)  QVERIFY(e->getWarpOutAt(i)  != nullptr);
    for(int i = 0; i < e->getWarpInSize(); i++)   QVERIFY(e->getWarpInAt(i)   != nullptr);
    for(int i = 0; i < e->getSignsSize(); i++)    QVERIFY(e->getSignsAt(i)    != nullptr);
    for(int i = 0; i < e->getSpritesSize(); i++)  QVERIFY(e->getSpritesAt(i)  != nullptr);
    for(int i = 0; i < e->getMonsRedSize(); i++)  QVERIFY(e->getMonsRedAt(i)  != nullptr);
    for(int i = 0; i < e->getMonsBlueSize(); i++) QVERIFY(e->getMonsBlueAt(i) != nullptr);
    for(int i = 0; i < e->getMonsWaterSize(); i++)QVERIFY(e->getMonsWaterAt(i)!= nullptr);
  }
}

QTEST_GUILESS_MAIN(TestDbEntryGetters)
#include "tst_db_entry_getters.moc"
