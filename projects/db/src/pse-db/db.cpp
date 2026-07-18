/*
  * Copyright 2020 Twilight
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
 * @file db.cpp
 * @brief Implementation of DB -- the aggregate and the load/index/deep-link
 *        bootstrap sequence. See db.h for the documented API.
 */

#include <QQmlEngine>
#include <QElapsedTimer>
#include <QDebug>
#include <pse-common/utility.h>

// db.h already includes all sub-database headers.
#include "db.h"

// ── Singleton ────────────────────────────────────────────────────────────────

DB* DB::inst()
{
  static DB* _inst = new DB;
  return _inst;
}

// ── Accessors ────────────────────────────────────────────────────────────────

GameData*        DB::json()         const { return GameData::inst(); }
BlocksDB*        DB::blocks()       const { return BlocksDB::inst(); }
CreditsDB*       DB::credits()      const { return CreditsDB::inst(); }
EventPokemonDB*  DB::eventPokemon() const { return EventPokemonDB::inst(); }
EventsDB*        DB::events()       const { return EventsDB::inst(); }
Examples*        DB::examples()     const { return Examples::inst(); }
Names*           DB::names()        const { return Names::inst(); }
FlyDB*           DB::fly()          const { return FlyDB::inst(); }
FontsDB*         DB::fonts()        const { return FontsDB::inst(); }
GameCornerDB*    DB::gameCorner()   const { return GameCornerDB::inst(); }
HiddenCoinsDB*   DB::hiddenCoins()  const { return HiddenCoinsDB::inst(); }
HiddenItemsDB*   DB::hiddenItems()  const { return HiddenItemsDB::inst(); }
ItemsDB*         DB::items()        const { return ItemsDB::inst(); }
MapsDB*          DB::maps()         const { return MapsDB::inst(); }
MissablesDB*     DB::missables()    const { return MissablesDB::inst(); }
MovesDB*         DB::moves()        const { return MovesDB::inst(); }
MusicDB*         DB::music()        const { return MusicDB::inst(); }
PokemonDB*       DB::pokemon()      const { return PokemonDB::inst(); }
ScriptsDB*       DB::scripts()      const { return ScriptsDB::inst(); }
SpriteSetDB*     DB::spriteSets()   const { return SpriteSetDB::inst(); }
SpritesDB*       DB::sprites()      const { return SpritesDB::inst(); }
StarterPokemonDB* DB::starters()    const { return StarterPokemonDB::inst(); }
TilesetDB*       DB::tilesets()     const { return TilesetDB::inst(); }
TileTraitsDB*    DB::tileTraits()   const { return TileTraitsDB::inst(); }
TmHmsDB*         DB::tmHms()        const { return TmHmsDB::inst(); }
TradesDB*        DB::trades()       const { return TradesDB::inst(); }
TrainersDB*      DB::trainers()     const { return TrainersDB::inst(); }
TypesDB*         DB::types()        const { return TypesDB::inst(); }

// ── Bootstrap ────────────────────────────────────────────────────────────────

DB::DB()
{
  initRes();
  qmlRegister();
  loadAll();
  indexAll();
  deepLinkAll();
}

void DB::initRes() const
{
  Q_INIT_RESOURCE(db);
}

void DB::qmlRegister() const
{
  static bool registered = false;
  if (registered) return;
  qmlRegisterUncreatableType<DB>("PSE.DB", 1, 0, "DB", "Can't instantiate in QML");
  registered = true;
}

void DB::loadAll() const
{
  static bool once = false;
  if (once) return;

  QElapsedTimer t;
  t.start();
  auto lap = [&](const char* name) {
    qDebug() << "[DB::loadAll]" << name << "—" << t.elapsed() << "ms";
    t.restart();
  };

  TypesDB::inst()->load();        lap("TypesDB");
  SpritesDB::inst()->load();      lap("SpritesDB");
  MusicDB::inst()->load();        lap("MusicDB");
  TilesetDB::inst()->load();      lap("TilesetDB");
  TileTraitsDB::inst()->load();   lap("TileTraitsDB");
  TrainersDB::inst()->load();     lap("TrainersDB");
  ItemsDB::inst()->load();        lap("ItemsDB");
  MovesDB::inst()->load();        lap("MovesDB");
  PokemonDB::inst()->load();      lap("PokemonDB");
  TmHmsDB::inst()->load();        lap("TmHmsDB");
  TradesDB::inst()->load();       lap("TradesDB");
  StarterPokemonDB::inst()->load(); lap("StarterPokemonDB");
  SpriteSetDB::inst()->load();    lap("SpriteSetDB");
  ScriptsDB::inst()->load();      lap("ScriptsDB");
  Examples::inst();               lap("Examples");
  Names::inst();                  lap("Names");
  CreditsDB::inst()->load();      lap("CreditsDB");
  EventPokemonDB::inst()->load(); lap("EventPokemonDB");
  EventsDB::inst()->load();       lap("EventsDB");
  FlyDB::inst()->load();          lap("FlyDB");
  FontsDB::inst()->load();        lap("FontsDB");
  GameCornerDB::inst()->load();   lap("GameCornerDB");
  HiddenCoinsDB::inst()->load();  lap("HiddenCoinsDB");
  HiddenItemsDB::inst()->load();  lap("HiddenItemsDB");
  MapsDB::inst()->load();         lap("MapsDB");
  MapStatesDB::inst()->load();    lap("MapStatesDB");
  MissablesDB::inst()->load();    lap("MissablesDB");
  BlocksDB::inst()->load();       lap("BlocksDB");

  once = true;
}

void DB::indexAll() const
{
  static bool once = false;
  if (once) return;

  QElapsedTimer t;
  t.start();
  auto lap = [&](const char* name) {
    qDebug() << "[DB::indexAll]" << name << "—" << t.elapsed() << "ms";
    t.restart();
  };

  TypesDB::inst()->index();    lap("TypesDB");
  SpritesDB::inst()->index();  lap("SpritesDB");
  MusicDB::inst()->index();    lap("MusicDB");
  TilesetDB::inst()->index();  lap("TilesetDB");
  TrainersDB::inst()->index(); lap("TrainersDB");
  ItemsDB::inst()->index();    lap("ItemsDB");
  MovesDB::inst()->index();    lap("MovesDB");
  PokemonDB::inst()->index();  lap("PokemonDB");
  SpriteSetDB::inst()->index(); lap("SpriteSetDB");
  ScriptsDB::inst()->index();  lap("ScriptsDB");
  EventsDB::inst()->index();   lap("EventsDB");
  FlyDB::inst()->index();      lap("FlyDB");
  FontsDB::inst()->index();    lap("FontsDB");
  MapsDB::inst()->index();     lap("MapsDB");
  MissablesDB::inst()->index(); lap("MissablesDB");

  once = true;
}

void DB::deepLinkAll() const
{
  static bool once = false;
  if (once) return;

  QElapsedTimer t;
  t.start();
  auto lap = [&](const char* name) {
    qDebug() << "[DB::deepLinkAll]" << name << "—" << t.elapsed() << "ms";
    t.restart();
  };

  MovesDB::inst()->deepLink();          lap("MovesDB");
  PokemonDB::inst()->deepLink();        lap("PokemonDB");
  TmHmsDB::inst()->deepLink();          lap("TmHmsDB");
  TradesDB::inst()->deepLink();         lap("TradesDB");
  StarterPokemonDB::inst()->deepLink(); lap("StarterPokemonDB");
  SpriteSetDB::inst()->deepLink();        lap("SpriteSetDB");
  ScriptsDB::inst()->deepLink();        lap("ScriptsDB");
  EventsDB::inst()->deepLink();         lap("EventsDB");
  FlyDB::inst()->deepLink();            lap("FlyDB");
  GameCornerDB::inst()->deepLink();     lap("GameCornerDB");
  HiddenCoinsDB::inst()->deepLink();    lap("HiddenCoinsDB");
  HiddenItemsDB::inst()->deepLink();    lap("HiddenItemsDB");

  // MapsDB is deep-linked LAST, and it must be deep-linked at all: without this call every map's
  // getToMap() / getToSprite() / getToMusic() is null, which is why AreaAudio::setTo() has quietly
  // been writing 0/0 for the music, and why map randomize was commented out. It resolves against
  // the maps/sprites/music/tileset indexes, so it has to run after index() (it does -- indexAll()
  // is called first) and after the other links. Confirmed safe: tst_sprite_data resolves all 918
  // sprites. See notes/plans/map-screen.md -> Phase 0.
  MapsDB::inst()->deepLink();           lap("MapsDB");

  // Missables resolve INTO maps (toMap/toMapSprite), so they link after MapsDB.
  // This call was missing entirely until 2026-07-16 — every missable's map link
  // was silently null (the Map Storage panel's missables section needs them).
  MissablesDB::inst()->deepLink();      lap("MissablesDB");
}

// ── QML ownership / context ──────────────────────────────────────────────────

void DB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);

  GameData::inst()->qmlProtect(engine);
  BlocksDB::inst()->qmlProtect(engine);
  CreditsDB::inst()->qmlProtect(engine);
  EventPokemonDB::inst()->qmlProtect(engine);
  EventsDB::inst()->qmlProtect(engine);
  Examples::inst()->qmlProtect(engine);
  Names::inst()->qmlProtect(engine);
  FlyDB::inst()->qmlProtect(engine);
  FontsDB::inst()->qmlProtect(engine);
  GameCornerDB::inst()->qmlProtect(engine);
  HiddenCoinsDB::inst()->qmlProtect(engine);
  HiddenItemsDB::inst()->qmlProtect(engine);
  ItemsDB::inst()->qmlProtect(engine);
  MapsDB::inst()->qmlProtect(engine);
  MissablesDB::inst()->qmlProtect(engine);
  MovesDB::inst()->qmlProtect(engine);
  MusicDB::inst()->qmlProtect(engine);
  PokemonDB::inst()->qmlProtect(engine);
  ScriptsDB::inst()->qmlProtect(engine);
  SpriteSetDB::inst()->qmlProtect(engine);
  SpritesDB::inst()->qmlProtect(engine);
  StarterPokemonDB::inst()->qmlProtect(engine);
  TilesetDB::inst()->qmlProtect(engine);
  TileTraitsDB::inst()->qmlProtect(engine);
  TmHmsDB::inst()->qmlProtect(engine);
  TradesDB::inst()->qmlProtect(engine);
  TrainersDB::inst()->qmlProtect(engine);
  TypesDB::inst()->qmlProtect(engine);
}

void DB::qmlHook(QQmlContext* const context) const
{
  // For some reason this demands it not be const
  context->setContextProperty("pseDB", const_cast<DB*>(this));
}

