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
 * @file tst_db_entry_getters2.cpp
 * @brief Getter sweeps for the db entry types tst_db_entry_getters doesn't cover:
 *        FontDBEntry, EventDBEntry, MissableDBEntry, EventPokemonDBEntry, the
 *        resolved MapDBEntry links (toTileset/toMusic/toSpriteSet + the wild-mon /
 *        pointer getters), and the MapDBEntryConnect sub-entry getters. Each getter
 *        is called over the whole store so its line is executed; deepLink() is run
 *        first so the resolved-link getters take their non-null path too.
 */

#include <QtTest>
#include <QQmlEngine>
#include <QQmlContext>

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/eventsdb.h>
#include <pse-db/missablesdb.h>
#include <pse-db/eventpokemondb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/flydb.h>
#include <pse-db/gamecornerdb.h>
#include <pse-db/hiddenItemsdb.h>
#include <pse-db/entries/flydbentry.h>
#include <pse-db/entries/gamecornerdbentry.h>
#include <pse-db/entries/hiddenitemdbentry.h>
#include <pse-db/entries/mapdbentrywarpin.h>
#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>
#include <pse-db/entries/mapdbentryspritepokemon.h>
#include <pse-db/entries/mapdbentryspritetrainer.h>
#include <pse-db/entries/mapdbentryspriteitem.h>
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
  void smallEntry_getters();
  void dbBase_accessorsAndQml();
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
      // Connection-rendering math (each direction's branch is hit across the maps).
      (void)c->stripLocation(); (void)c->mapPos(); (void)c->stripSize();
      (void)c->yAlign(); (void)c->xAlign(); (void)c->window();
    }

    // Sprite sub-entry getters.
    for(int i = 0; i < e->getSpritesSize(); i++) {
      const MapDBEntrySprite* s = e->getSpritesAt(i);
      if(s == nullptr) continue;
      (void)s->adjustedX(); (void)s->adjustedY(); (void)s->getSprite();
      (void)s->getX(); (void)s->getY(); (void)s->getMove(); (void)s->getText();
      (void)s->getRange(); (void)s->getFace(); (void)s->getMissable();
      (void)s->getToMissable(); (void)s->getToSprite(); (void)s->getParent();

      // Subtype getters, reached by casting on type().
      if(s->type() == MapDBEntrySprite::SpriteType::POKEMON) {
        auto* sp = static_cast<const MapDBEntrySpritePokemon*>(s);
        (void)sp->getPokemon(); (void)sp->getLevel(); (void)sp->getToPokemon();
      } else if(s->type() == MapDBEntrySprite::SpriteType::TRAINER) {
        auto* st = static_cast<const MapDBEntrySpriteTrainer*>(s);
        (void)st->getTrainerClass(); (void)st->getTeam(); (void)st->getToTrainer();
      } else if(s->type() == MapDBEntrySprite::SpriteType::ITEM) {
        auto* si = static_cast<const MapDBEntrySpriteItem*>(s);
        (void)si->getItem(); (void)si->getToItem();
      }
    }

    // Warp-in sub-entry getters.
    for(int i = 0; i < e->getWarpInSize(); i++) {
      const MapDBEntryWarpIn* w = e->getWarpInAt(i);
      if(w == nullptr) continue;
      (void)w->getX(); (void)w->getY(); (void)w->getParent();
      const int n = w->getToConnectingWarpsSize();
      for(int k = 0; k < n; k++) (void)w->getToConnectingWarpsAt(k);
      (void)w->getToConnectingWarps();
    }
  }
}

void TestDbEntryGetters2::smallEntry_getters()
{
  for(FlyDBEntry* e : FlyDB::inst()->getStore()) {
    if(e == nullptr) continue;
    (void)e->getName(); (void)e->getInd(); (void)e->getToMap();
  }
  for(GameCornerDBEntry* e : GameCornerDB::inst()->getStore()) {
    if(e == nullptr) continue;
    (void)e->getName(); (void)e->getType(); (void)e->getPrice();
    (void)e->getLevel(); (void)e->getToPokemon(); (void)e->getToItem();
  }
  for(HiddenItemDBEntry* e : HiddenItemsDB::inst()->getStore()) {
    if(e == nullptr) continue;
    (void)e->getMap(); (void)e->getX(); (void)e->getY(); (void)e->getToMap();
  }
  // ItemDBEntry getters past the ones tst_db_entry_getters already drives.
  for(ItemDBEntry* e : ItemsDB::inst()->getStore()) {
    if(e == nullptr) continue;
    (void)e->isGameCornerExclusive(); (void)e->getToGameCorner();
    (void)e->getToEvolvePokemon(); (void)e->getToTeachPokemon();
    for(int i = 0; i < e->getToEvolvePokemonSize(); i++) (void)e->getToEvolvePokemonAt(i);
    for(int i = 0; i < e->getToTeachPokemonSize(); i++)  (void)e->getToTeachPokemonAt(i);
  }
}

void TestDbEntryGetters2::dbBase_accessorsAndQml()
{
  DB* db = DB::inst();

  // The 26 sub-DB accessor one-liners (each returns its singleton).
  QVERIFY(db->json()         != nullptr);
  QVERIFY(db->credits()      != nullptr);
  QVERIFY(db->eventPokemon() != nullptr);
  QVERIFY(db->events()       != nullptr);
  QVERIFY(db->examples()     != nullptr);
  QVERIFY(db->names()        != nullptr);
  QVERIFY(db->fly()          != nullptr);
  QVERIFY(db->fonts()        != nullptr);
  QVERIFY(db->gameCorner()   != nullptr);
  QVERIFY(db->hiddenCoins()  != nullptr);
  QVERIFY(db->hiddenItems()  != nullptr);
  QVERIFY(db->items()        != nullptr);
  QVERIFY(db->maps()         != nullptr);
  QVERIFY(db->missables()    != nullptr);
  QVERIFY(db->moves()        != nullptr);
  QVERIFY(db->music()        != nullptr);
  QVERIFY(db->pokemon()      != nullptr);
  QVERIFY(db->scripts()      != nullptr);
  QVERIFY(db->spriteSets()   != nullptr);
  QVERIFY(db->sprites()      != nullptr);
  QVERIFY(db->starters()     != nullptr);
  QVERIFY(db->tilesets()     != nullptr);
  QVERIFY(db->tmHms()        != nullptr);
  QVERIFY(db->trades()       != nullptr);
  QVERIFY(db->trainers()     != nullptr);
  QVERIFY(db->types()        != nullptr);

  // qmlProtect cascades to every sub-DB (and thence every entry), and qmlHook
  // exposes DB as a context property. One call covers the qmlProtect path across
  // the whole db layer.
  QQmlEngine engine;
  db->qmlProtect(&engine);
  QCOMPARE(QQmlEngine::objectOwnership(db), QQmlEngine::CppOwnership);
  db->qmlHook(engine.rootContext());
  QCOMPARE(engine.rootContext()->contextProperty("pseDB").value<QObject*>(),
           static_cast<QObject*>(db));
}

QTEST_GUILESS_MAIN(TestDbEntryGetters2)
#include "tst_db_entry_getters2.moc"
