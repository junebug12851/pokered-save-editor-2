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

#include "../data/db/credits.h"
#include "../data/db/eventpokemon.h"
#include "../data/db/events.h"
#include "../data/db/examplesplayer.h"
#include "../data/db/examplespokemon.h"
#include "../data/db/examplesrival.h"
#include "../data/db/fly.h"
#include "../data/db/fonts.h"
#include "../data/db/hiddenCoins.h"
#include "../data/db/hiddenItems.h"
#include "../data/db/items.h"
#include "../data/db/maps.h"
#include "../data/db/missables.h"
#include "../data/db/moves.h"
#include "../data/db/music.h"
#include "../data/db/names.h"
#include "../data/db/namesPokemon.h"
#include "../data/db/pokemon.h"
#include "../data/db/scripts.h"
#include "../data/db/sprites.h"
#include "../data/db/spriteSet.h"
#include "../data/db/starterPokemon.h"
#include "../data/db/tileset.h"
#include "../data/db/tmHm.h"
#include "../data/db/trades.h"
#include "../data/db/trainers.h"
#include "../data/db/types.h"

// Step 1: Load all JSON data into memory, properly parsed
void load()
{
  CreditsDB::load();
  EventPokemonDB::load();
  EventsDB::load();
  ExamplesPlayer::load();
  ExamplesPokemon::load();
  ExamplesRival::load();
  FlyDB::load();
  FontsDB::load();
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
  load();
  index();
  deepLink();
}
