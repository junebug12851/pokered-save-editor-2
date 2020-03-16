/*
  * Copyright 2020 June Hanabi
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

#include <pse-db/db.h>
#include <pse-db/eventpokemondb.h>
#include <pse-db/eventsdb.h>
#include <pse-db/examplesplayer.h>
#include <pse-db/examplespokemon.h>
#include <pse-db/examplesrival.h>
#include <pse-db/flydb.h>
#include <pse-db/fontsdb.h>
#include <pse-db/gamecornerdb.h>
#include <pse-db/hiddenCoins.h>
#include <pse-db/hiddenItems.h>
#include <pse-db/items.h>
#include <pse-db/maps.h>
#include <pse-db/missables.h>
#include <pse-db/moves.h>
#include <pse-db/music.h>
#include <pse-db/names.h>
#include <pse-db/namesPokemon.h>
#include <pse-db/pokemon.h>
#include <pse-db/scripts.h>
#include <pse-db/sprites.h>
#include <pse-db/spriteSet.h>
#include <pse-db/starterPokemon.h>
#include <pse-db/tileset.h>
#include <pse-db/tmHm.h>
#include <pse-db/trades.h>
#include <pse-db/trainers.h>
#include <pse-db/types.h>

// Step 1: Load all JSON data into memory, properly parsed
void load()
{
  EventPokemonDB::load();
  EventsDB::load();
  ExamplesPlayer::load();
  ExamplesPokemon::load();
  ExamplesRival::load();
  FlyDB::load();
  FontsDB::load();
  GameCornerDB::load();
  HiddenCoinsDB::load();
  HiddenItemsDB::load();
  ItemsDB::load();
  MapsDB::load(); // <--- Fairly expensive
  MissablesDB::load();
  MovesDB::load();
  MusicDB::load();
  NamesDB::load();
  NamesPokemonDB::load();
  PokemonDB::load(); // <--- Also fairly expensive
  ScriptsDB::load();
  SpritesDB::load();
  SpriteSetDB::load();
  StarterPokemonDB::load();
  TilesetDB::load();
  TmHmsDB::load();
  TradesDB::load();
  TrainersDB::load();
  TypesDB::load();
}

// Step 2: Index most JSON data in memory making it rapidly accessible
// not everything can be indexed.
void index()
{
  EventsDB::index();
  FlyDB::index();
  FontsDB::index();
  ItemsDB::index();
  MapsDB::index();
  MissablesDB::index();
  MovesDB::index();
  MusicDB::index();
  PokemonDB::index();
  ScriptsDB::index();
  SpritesDB::index();
  SpriteSetDB::index();
  TilesetDB::index();
  TrainersDB::index();
  TypesDB::index();
}

// Step 3: Deep link most data to other data for rapid cross-data access
// Not all data can be deep-linked
void deepLink()
{
  EventPokemonDB::deepLink();
  EventsDB::deepLink();
  FlyDB::deepLink();
  GameCornerDB::deepLink();
  HiddenCoinsDB::deepLink();
  HiddenItemsDB::deepLink();
  ItemsDB::deepLink();
  MapsDB::deepLink(); // <-- Also now one of the most expensive operations!!!
  MissablesDB::deepLink();
  MovesDB::deepLink();
  PokemonDB::deepLink(); // <-- One the most expensive operations!!!
  ScriptsDB::deepLink();
  SpriteSetDB::deepLink();
  StarterPokemonDB::deepLink();
  TmHmsDB::deepLink();
  TradesDB::deepLink();
}

extern void bootDatabase()
{
  // The replacement for this function and all the code here
  DB::inst();

  load();
  index();
  deepLink();
}
