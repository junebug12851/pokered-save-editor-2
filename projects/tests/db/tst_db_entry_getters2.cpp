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
 * @file tst_db_entry_getters2.cpp
 * @brief Getter sweeps for the db entry types tst_db_entry_getters doesn't cover:
 *        FontDBEntry, EventDBEntry, MissableDBEntry, EventPokemonDBEntry, the
 *        resolved MapDBEntry links (toTileset/toMusic/toSpriteSet + the wild-mon /
 *        pointer getters), and the MapDBEntryConnect sub-entry getters. Each getter
 *        is called over the whole store so its line is executed; deepLink() is run
 *        first so the resolved-link getters take their non-null path too.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/eventsdb.h>
#include <pse-db/missablesdb.h>
#include <pse-db/eventpokemondb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/entries/fontdbentry.h>
#include <pse-db/entries/eventdbentry.h>
#include <pse-db/entries/missabledbentry.h>
#include <pse-db/entries/eventpokemondbentry.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/entries/mapdbentryconnect.h>
#include <pse-db/entries/mapdbentrysprite.h>

class TestDbEntryGetters2 : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void fontEntry_getters();
  void eventEntry_getters();
  void missableEntry_getters();
  void eventPokemonEntry_getters();
  void mapEntry_resolvedLinksAndConnects();
};

void TestDbEntryGetters2::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  MapsDB::inst()->deepLink();   // resolve map links so getTo* take their non-null path
}

void TestDbEntryGetters2::fontEntry_getters()
{
  const auto store = FontsDB::inst()->getStore();
  QVERIFY(!store.isEmpty());
  for(FontDBEntry* e : store) {
    QVERIFY(e != nullptr);
    QVERIFY(e->getInd() >= 0);
    (void)e->getName(); (void)e->getShorthand(); (void)e->getPicture();
    (void)e->getLength(); (void)e->getAlias(); (void)e->getTip();
    (void)e->getControl(); (void)e->getMultiChar(); (void)e->getVariable();
    (void)e->getSingleChar(); (void)e->getNormal();
  }
}

void TestDbEntryGetters2::eventEntry_getters()
{
  const auto store = EventsDB::inst()->getStore();
  QVERIFY(!store.isEmpty());
  for(EventDBEntry* e : store) {
    QVERIFY(e != nullptr);
    (void)e->getName(); (void)e->getInd(); (void)e->getByte(); (void)e->getBit();
    const int n = e->getMapsSize();
    QVERIFY(n >= 0);
    for(int i = 0; i < n; i++) (void)e->getMapAt(i);
    (void)e->getMaps();
    const int tn = e->getToMapsSize();
    QVERIFY(tn >= 0);
    for(int i = 0; i < tn; i++) (void)e->getToMapAt(i);
    (void)e->getToMaps();
  }
}

void TestDbEntryGetters2::missableEntry_getters()
{
  const auto store = MissablesDB::inst()->getStore();
  QVERIFY(!store.isEmpty());
  for(MissableDBEntry* e : store) {
    QVERIFY(e != nullptr);
    (void)e->getName(); (void)e->getInd(); (void)e->getMap(); (void)e->getSprite();
    (void)e->getDefShow(); (void)e->getToMap(); (void)e->getToMapSprite();
  }
}

void TestDbEntryGetters2::eventPokemonEntry_getters()
{
  const auto store = EventPokemonDB::inst()->getStore();
  QVERIFY(!store.isEmpty());
  for(EventPokemonDBEntry* e : store) {
    QVERIFY(e != nullptr);
    (void)e->getTitle(); (void)e->getDesc(); (void)e->getPokemon(); (void)e->getOtName();
    (void)e->getRegion(); (void)e->getMoves(); (void)e->getToPokemon(); (void)e->getLevel();
    (void)e->getOtId(); (void)e->getDvAtk(); (void)e->getDvDef(); (void)e->getDvSpd();
    (void)e->getDvSp();
  }
}

void TestDbEntryGetters2::mapEntry_resolvedLinksAndConnects()
{
  MapsDB* db = MapsDB::inst();
  const int maps = db->getStoreSize();
  QVERIFY(maps > 0);

  for(int m = 0; m < maps; m++) {
    MapDBEntry* e = db->getStoreAt(m);
    if(e == nullptr) continue;

    // Pointer + size scalar getters, and the resolved links (may be null).
    (void)e->getBorder(); (void)e->getDataPtr(); (void)e->getTextPtr(); (void)e->getScriptPtr();
    (void)e->width2X2(); (void)e->height2X2();
    (void)e->getToTileset(); (void)e->getToMusic(); (void)e->getToSpriteSet();

    // Wild-mon lists (full getters + the size/At already in the other test).
    (void)e->getMonsRed(); (void)e->getMonsBlue(); (void)e->getMonsWater();

    // Edge connections: drive every MapDBEntryConnect getter.
    const auto connects = e->getConnect();
    for(MapDBEntryConnect* c : connects) {
      if(c == nullptr) continue;
      (void)c->getDir(); (void)c->getMap(); (void)c->getStripMove();
      (void)c->getStripOffset(); (void)c->getFlag(); (void)c->getToMap();
      (void)c->getFromMap(); (void)c->getParent();
    }

    // Sprite placement helpers.
    for(int i = 0; i < e->getSpritesSize(); i++) {
      const MapDBEntrySprite* s = e->getSpritesAt(i);
      if(s == nullptr) continue;
      (void)s->adjustedX(); (void)s->adjustedY();
    }
  }
}

QTEST_GUILESS_MAIN(TestDbEntryGetters2)
#include "tst_db_entry_getters2.moc"
