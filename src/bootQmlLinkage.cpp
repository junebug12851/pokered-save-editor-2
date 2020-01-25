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

#include <QString>
#include <QQmlEngine>

#include "./data/file/filemanagement.h"
#include "./data/file/savefile.h"
#include "./data/file/expanded/daycare.h"
#include "./data/file/expanded/halloffame.h"
#include "./data/file/expanded/rival.h"
#include "./data/file/expanded/savefileexpanded.h"
#include "./data/file/expanded/storage.h"

#include "./data/file/expanded/area/area.h"
#include "./data/file/expanded/area/areaaudio.h"
#include "./data/file/expanded/area/areageneral.h"
#include "./data/file/expanded/area/arealoadedsprites.h"
#include "./data/file/expanded/area/areamap.h"
#include "./data/file/expanded/area/areanpc.h"
#include "./data/file/expanded/area/areaplayer.h"
#include "./data/file/expanded/area/areapokemon.h"
#include "./data/file/expanded/area/areapuzzle.h"
#include "./data/file/expanded/area/areasign.h"
#include "./data/file/expanded/area/areasprites.h"
#include "./data/file/expanded/area/areatileset.h"
#include "./data/file/expanded/area/areawarps.h"

#include "./data/file/expanded/fragments/hofpokemon.h"
#include "./data/file/expanded/fragments/hofrecord.h"
#include "./data/file/expanded/fragments/item.h"
#include "./data/file/expanded/fragments/itemstoragebox.h"
#include "./data/file/expanded/fragments/mapconndata.h"
#include "./data/file/expanded/fragments/pokemonbox.h"
#include "./data/file/expanded/fragments/pokemonparty.h"
#include "./data/file/expanded/fragments/pokemonstoragebox.h"
#include "./data/file/expanded/fragments/pokemonstorageset.h"
#include "./data/file/expanded/fragments/signdata.h"
#include "./data/file/expanded/fragments/spritedata.h"
#include "./data/file/expanded/fragments/warpdata.h"

#include "./data/file/expanded/player/player.h"
#include "./data/file/expanded/player/playerbasics.h"
#include "./data/file/expanded/player/playeritems.h"
#include "./data/file/expanded/player/playerpokedex.h"
#include "./data/file/expanded/player/playerpokemon.h"

#include "./data/file/expanded/world/world.h"
#include "./data/file/expanded/world/worldcompleted.h"
#include "./data/file/expanded/world/worldevents.h"
#include "./data/file/expanded/world/worldgeneral.h"
#include "./data/file/expanded/world/worldhidden.h"
#include "./data/file/expanded/world/worldmissables.h"
#include "./data/file/expanded/world/worldother.h"
#include "./data/file/expanded/world/worldscripts.h"
#include "./data/file/expanded/world/worldtowns.h"
#include "./data/file/expanded/world/worldtrades.h"

constexpr const char* msg = "Can't instantiate in QML";

const char* dn(QString name)
{
  return ("app." + name.toLower()).toLocal8Bit().data();
}

extern void bootQmlLinkage()
{
  // Can't put this into a template because there would be no QML processing
  // for hints so I have to duplicate the class name 3 times on each line

  // Creatable Types
  // Enums are allowed to be created by QML
  qmlRegisterType<ContrastIds>(dn("ContrastIds"), 1, 0, "ContrastIds");
  qmlRegisterType<PlayerDir>(dn("PlayerDir"), 1, 0, "PlayerDir");
  qmlRegisterType<PokemonStats>(dn("PokemonStats"), 1, 0, "PokemonStats");
  qmlRegisterType<PokemonRandom>(dn("PokemonRandom"), 1, 0, "PokemonRandom");
  qmlRegisterType<SpriteMovementStatus>(dn("SpriteMovementStatus"), 1, 0, "SpriteMovementStatus");
  qmlRegisterType<SpriteFacing>(dn("SpriteFacing"), 1, 0, "SpriteFacing");
  qmlRegisterType<SpriteMobility>(dn("SpriteMobility"), 1, 0, "SpriteMobility");
  qmlRegisterType<SpriteMovement>(dn("SpriteMovement"), 1, 0, "SpriteMovement");
  qmlRegisterType<SpriteGrass>(dn("SpriteGrass"), 1, 0, "SpriteGrass");
  qmlRegisterType<Badges>(dn("Badges"), 1, 0, "Badges");

  // Uncreatable Types
  // Expanded Data is meant to be interacted with not created by QML
  qmlRegisterUncreatableType<FileManagement>(dn("FileManagement"), 1, 0, "FileManagement", msg);
  qmlRegisterUncreatableType<SaveFile>(dn("SaveFile"), 1, 0, "SaveFile", msg);
  qmlRegisterUncreatableType<Daycare>(dn("Daycare"), 1, 0, "Daycare", msg);
  qmlRegisterUncreatableType<HallOfFame>(dn("HallOfFame"), 1, 0, "HallOfFame", msg);
  qmlRegisterUncreatableType<Rival>(dn("Rival"), 1, 0, "Rival", msg);
  qmlRegisterUncreatableType<SaveFileExpanded>(dn("SaveFileExpanded"), 1, 0, "SaveFileExpanded", msg);
  qmlRegisterUncreatableType<Storage>(dn("Storage"), 1, 0, "Storage", msg);

  qmlRegisterUncreatableType<Area>(dn("Area"), 1, 0, "Area", msg);
  qmlRegisterUncreatableType<AreaAudio>(dn("AreaAudio"), 1, 0, "AreaAudio", msg);
  qmlRegisterUncreatableType<AreaGeneral>(dn("AreaGeneral"), 1, 0, "AreaGeneral", msg);
  qmlRegisterUncreatableType<AreaLoadedSprites>(dn("AreaLoadedSprites"), 1, 0, "AreaLoadedSprites", msg);
  qmlRegisterUncreatableType<AreaMap>(dn("AreaMap"), 1, 0, "AreaMap", msg);
  qmlRegisterUncreatableType<AreaNPC>(dn("AreaNPC"), 1, 0, "AreaNPC", msg);
  qmlRegisterUncreatableType<AreaPlayer>(dn("AreaPlayer"), 1, 0, "AreaPlayer", msg);
  qmlRegisterUncreatableType<AreaPokemonWild>(dn("AreaPokemonWild"), 1, 0, "AreaPokemonWild", msg);
  qmlRegisterUncreatableType<AreaPokemon>(dn("AreaPokemon"), 1, 0, "AreaPokemon", msg);
  qmlRegisterUncreatableType<AreaPuzzle>(dn("AreaPuzzle"), 1, 0, "AreaPuzzle", msg);
  qmlRegisterUncreatableType<AreaSign>(dn("AreaSign"), 1, 0, "AreaSign", msg);
  qmlRegisterUncreatableType<AreaSprites>(dn("AreaSprites"), 1, 0, "AreaSprites", msg);
  qmlRegisterUncreatableType<AreaTileset>(dn("AreaTileset"), 1, 0, "AreaTileset", msg);
  qmlRegisterUncreatableType<AreaWarps>(dn("AreaWarps"), 1, 0, "AreaWarps", msg);

  qmlRegisterUncreatableType<HoFPokemon>(dn("HoFPokemon"), 1, 0, "HoFPokemon", msg);
  qmlRegisterUncreatableType<HoFRecord>(dn("HoFRecord"), 1, 0, "HoFRecord", msg);
  qmlRegisterUncreatableType<Item>(dn("Item"), 1, 0, "Item", msg);
  qmlRegisterUncreatableType<ItemStorageBox>(dn("ItemStorageBox"), 1, 0, "ItemStorageBox", msg);
  qmlRegisterUncreatableType<MapConnData>(dn("MapConnData"), 1, 0, "MapConnData", msg);
  qmlRegisterUncreatableType<PokemonMove>(dn("PokemonMove"), 1, 0, "PokemonMove", msg);
  qmlRegisterUncreatableType<PokemonBox>(dn("PokemonBox"), 1, 0, "PokemonBox", msg);
  qmlRegisterUncreatableType<PokemonParty>(dn("PokemonParty"), 1, 0, "PokemonParty", msg);
  qmlRegisterUncreatableType<PokemonStorageBox>(dn("PokemonStorageBox"), 1, 0, "PokemonStorageBox", msg);
  qmlRegisterUncreatableType<PokemonStorageSet>(dn("PokemonStorageSet"), 1, 0, "PokemonStorageSet", msg);
  qmlRegisterUncreatableType<SignData>(dn("SignData"), 1, 0, "SignData", msg);
  qmlRegisterUncreatableType<SpriteData>(dn("SpriteData"), 1, 0, "SpriteData", msg);
  qmlRegisterUncreatableType<WarpData>(dn("WarpData"), 1, 0, "WarpData", msg);

  qmlRegisterUncreatableType<Player>(dn("Player"), 1, 0, "Player", msg);
  qmlRegisterUncreatableType<PlayerBasics>(dn("PlayerBasics"), 1, 0, "PlayerBasics", msg);
  qmlRegisterUncreatableType<PlayerItems>(dn("PlayerItems"), 1, 0, "PlayerItems", msg);
  qmlRegisterUncreatableType<PlayerPokedex>(dn("PlayerPokedex"), 1, 0, "PlayerPokedex", msg);
  qmlRegisterUncreatableType<PlayerPokemon>(dn("PlayerPokemon"), 1, 0, "PlayerPokemon", msg);

  qmlRegisterUncreatableType<World>(dn("World"), 1, 0, "World", msg);
  qmlRegisterUncreatableType<WorldCompleted>(dn("WorldCompleted"), 1, 0, "WorldCompleted", msg);
  qmlRegisterUncreatableType<WorldEvents>(dn("WorldEvents"), 1, 0, "WorldEvents", msg);
  qmlRegisterUncreatableType<Options>(dn("Options"), 1, 0, "Options", msg);
  qmlRegisterUncreatableType<LetterDelay>(dn("LetterDelay"), 1, 0, "LetterDelay", msg);
  qmlRegisterUncreatableType<WorldGeneral>(dn("WorldGeneral"), 1, 0, "WorldGeneral", msg);
  qmlRegisterUncreatableType<WorldHidden>(dn("WorldHidden"), 1, 0, "WorldHidden", msg);
  qmlRegisterUncreatableType<WorldMissables>(dn("WorldMissables"), 1, 0, "WorldMissables", msg);
  qmlRegisterUncreatableType<Playtime>(dn("Playtime"), 1, 0, "Playtime", msg);
  qmlRegisterUncreatableType<WorldOther>(dn("WorldOther"), 1, 0, "WorldOther", msg);
  qmlRegisterUncreatableType<WorldScripts>(dn("WorldScripts"), 1, 0, "WorldScripts", msg);
  qmlRegisterUncreatableType<WorldTowns>(dn("WorldTowns"), 1, 0, "WorldTowns", msg);
  qmlRegisterUncreatableType<WorldTrades>(dn("WorldTrades"), 1, 0, "WorldTrades", msg);
}
