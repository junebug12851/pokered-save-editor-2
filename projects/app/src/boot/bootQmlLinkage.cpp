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

#include <pse-common/utility.h>

#include "../bridge/bridge.h"
#include "../bridge/settings.h"
#include "../bridge/router.h"

#include "../mvc/itemstoragemodel.h"
#include "../mvc/pokedexmodel.h"
#include "../mvc/itemmarketmodel.h"
#include "../mvc/pokemonboxselectmodel.h"
#include "../mvc/pokemonstoragemodel.h"
#include "../mvc/typesmodel.h"
#include "../mvc/speciesselectmodel.h"
#include "../mvc/statusselectmodel.h"
#include "../mvc/natureselectmodel.h"
#include "../mvc/moveselectmodel.h"
#include "../mvc/mapselectmodel.h"

#include <pse-db/examplesplayer.h>
#include <pse-db/examplespokemon.h>
#include <pse-db/examplesrival.h>
#include <pse-db/fonts.h>
#include <pse-db/fontsearch.h>
#include <pse-db/names.h>
#include <pse-db/namesPokemon.h>

#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/daycare.h>
#include <pse-savefile/expanded/halloffame.h>
#include <pse-savefile/expanded/rival.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/storage.h>

#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areaaudio.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/arealoadedsprites.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areanpc.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areapokemon.h>
#include <pse-savefile/expanded/world/worldlocal.h>
#include <pse-savefile/expanded/area/areasign.h>
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/area/areawarps.h>

#include <pse-savefile/expanded/fragments/hofpokemon.h>
#include <pse-savefile/expanded/fragments/hofrecord.h>
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/mapconndata.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>
#include <pse-savefile/expanded/fragments/pokemonstorageset.h>
#include <pse-savefile/expanded/fragments/signdata.h>
#include <pse-savefile/expanded/fragments/spritedata.h>
#include <pse-savefile/expanded/fragments/warpdata.h>

#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/player/playerpokedex.h>
#include <pse-savefile/expanded/player/playerpokemon.h>

#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldcompleted.h>
#include <pse-savefile/expanded/world/worldevents.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <pse-savefile/expanded/world/worldhidden.h>
#include <pse-savefile/expanded/world/worldmissables.h>
#include <pse-savefile/expanded/world/worldother.h>
#include <pse-savefile/expanded/world/worldscripts.h>
#include <pse-savefile/expanded/world/worldtowns.h>
#include <pse-savefile/expanded/world/worldtrades.h>

extern void bootQmlLinkage()
{
  // Can't put this into a template because there would be no QML processing
  // for hints so I have to duplicate the class name 3 times on each line
  // EDIT: Apparently I literally can't have any help here, any sort of help
  // doesn't work with Qt's Meta System. I have to verbatim spell everything out.
  // No variables, functions, templates, none of that. It's straight up copying
  // throughout

  // Thanks Regex101 regex101.com
  // Qt won't let me have any help from C++ but Regex101 allowed me to easily
  // make all the duplicating without errors and time consuming

  // Creatable Types
  // Enums are allowed to be created by QML
  qmlRegisterType<ContrastIds>("App.ContrastIds", 1, 0, "ContrastIds");
  qmlRegisterType<PlayerDir>("App.PlayerDir", 1, 0, "PlayerDir");
  qmlRegisterType<PokemonStats>("App.PokemonStats", 1, 0, "PokemonStats");
  qmlRegisterType<PokemonNatures>("App.PokemonNatures", 1, 0, "PokemonNatures");
  qmlRegisterType<PokemonRandom>("App.PokemonRandom", 1, 0, "PokemonRandom");
  qmlRegisterType<SpriteMovementStatus>("App.SpriteMovementStatus", 1, 0, "SpriteMovementStatus");
  qmlRegisterType<SpriteFacing>("App.SpriteFacing", 1, 0, "SpriteFacing");
  qmlRegisterType<SpriteMobility>("App.SpriteMobility", 1, 0, "SpriteMobility");
  qmlRegisterType<SpriteMovement>("App.SpriteMovement", 1, 0, "SpriteMovement");
  qmlRegisterType<SpriteGrass>("App.SpriteGrass", 1, 0, "SpriteGrass");
  qmlRegisterType<Badges>("App.Badges", 1, 0, "Badges");

  // Uncreatable Types
  qmlRegisterUncreatableType<Bridge>("App.Bridge", 1, 0, "Bridge", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Router>("App.Router", 1, 0, "Router", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Settings>("App.Settings", 1, 0, "Settings", "Can't instantiate in QML");

  qmlRegisterUncreatableType<Utility>("App.Utility", 1, 0, "Utility", "Can't instantiate in QML");
  qmlRegisterUncreatableType<ItemStorageModel>("App.ItemStorageModel", 1, 0, "ItemStorageModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokedexModel>("App.PokedexModel", 1, 0, "PokedexModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<ItemMarketModel>("App.MarketModel", 1, 0, "MarketModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokemonStorageModel>("App.PokemonStorageModel", 1, 0, "PokemonStorageModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokemonBoxSelectModel>("App.PokemonBoxSelectModel", 1, 0, "PokemonBoxSelectModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<TypesModel>("App.TypesModel", 1, 0, "TypesModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<SpeciesSelectModel>("App.SpeciesSelectModel", 1, 0, "SpeciesSelectModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<StatusSelectModel>("App.StatusSelectModel", 1, 0, "StatusSelectModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<NatureSelectModel>("App.NatureSelectModel", 1, 0, "NatureSelectModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<MoveSelectModel>("App.MoveSelectModel", 1, 0, "MoveSelectModel", "Can't instantiate in QML");
  qmlRegisterUncreatableType<MapSelectModel>("App.MapSelectModel", 1, 0, "MapSelectModel", "Can't instantiate in QML");

  qmlRegisterUncreatableType<ExamplesPlayer>("App.ExamplesPlayer", 1, 0, "ExamplesPlayer", "Can't instantiate in QML");
  qmlRegisterUncreatableType<ExamplesPokemon>("App.ExamplesPokemon", 1, 0, "ExamplesPokemon", "Can't instantiate in QML");
  qmlRegisterUncreatableType<ExamplesRival>("App.ExamplesRival", 1, 0, "ExamplesRival", "Can't instantiate in QML");
  qmlRegisterUncreatableType<FontSearch>("App.FontSearch", 1, 0, "FontSearch", "Can't instantiate in QML");
  qmlRegisterUncreatableType<FontsDB>("App.FontsDB", 1, 0, "FontsDB", "Can't instantiate in QML");
  qmlRegisterUncreatableType<FontDBEntry>("App.FontDBEntry", 1, 0, "FontDBEntry", "Can't instantiate in QML");
  qmlRegisterUncreatableType<NamesDB>("App.NamesDB", 1, 0, "NamesDB", "Can't instantiate in QML");
  qmlRegisterUncreatableType<NamesPokemonDB>("App.NamesPokemonDB", 1, 0, "NamesPokemonDB", "Can't instantiate in QML");

  qmlRegisterUncreatableType<FileManagement>("App.FileManagement", 1, 0, "FileManagement", "Can't instantiate in QML");
  qmlRegisterUncreatableType<SaveFile>("App.SaveFile", 1, 0, "SaveFile", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Daycare>("App.Daycare", 1, 0, "Daycare", "Can't instantiate in QML");
  qmlRegisterUncreatableType<HallOfFame>("App.HallOfFame", 1, 0, "HallOfFame", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Rival>("App.Rival", 1, 0, "Rival", "Can't instantiate in QML");
  qmlRegisterUncreatableType<SaveFileExpanded>("App.SaveFileExpanded", 1, 0, "SaveFileExpanded", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Storage>("App.Storage", 1, 0, "Storage", "Can't instantiate in QML");

  qmlRegisterUncreatableType<Area>("App.Area", 1, 0, "Area", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaAudio>("App.AreaAudio", 1, 0, "AreaAudio", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaGeneral>("App.AreaGeneral", 1, 0, "AreaGeneral", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaLoadedSprites>("App.AreaLoadedSprites", 1, 0, "AreaLoadedSprites", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaMap>("App.AreaMap", 1, 0, "AreaMap", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaNPC>("App.AreaNPC", 1, 0, "AreaNPC", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaPlayer>("App.AreaPlayer", 1, 0, "AreaPlayer", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaPokemonWild>("App.AreaPokemonWild", 1, 0, "AreaPokemonWild", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaPokemon>("App.AreaPokemon", 1, 0, "AreaPokemon", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaSign>("App.AreaSign", 1, 0, "AreaSign", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaSprites>("App.AreaSprites", 1, 0, "AreaSprites", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaTileset>("App.AreaTileset", 1, 0, "AreaTileset", "Can't instantiate in QML");
  qmlRegisterUncreatableType<AreaWarps>("App.AreaWarps", 1, 0, "AreaWarps", "Can't instantiate in QML");

  qmlRegisterUncreatableType<HoFPokemon>("App.HoFPokemon", 1, 0, "HoFPokemon", "Can't instantiate in QML");
  qmlRegisterUncreatableType<HoFRecord>("App.HoFRecord", 1, 0, "HoFRecord", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Item>("App.Item", 1, 0, "Item", "Can't instantiate in QML");
  qmlRegisterUncreatableType<ItemStorageBox>("App.ItemStorageBox", 1, 0, "ItemStorageBox", "Can't instantiate in QML");
  qmlRegisterUncreatableType<MapConnData>("App.MapConnData", 1, 0, "MapConnData", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokemonMove>("App.PokemonMove", 1, 0, "PokemonMove", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokemonBox>("App.PokemonBox", 1, 0, "PokemonBox", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokemonParty>("App.PokemonParty", 1, 0, "PokemonParty", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokemonStorageBox>("App.PokemonStorageBox", 1, 0, "PokemonStorageBox", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PokemonStorageSet>("App.PokemonStorageSet", 1, 0, "PokemonStorageSet", "Can't instantiate in QML");
  qmlRegisterUncreatableType<SignData>("App.SignData", 1, 0, "SignData", "Can't instantiate in QML");
  qmlRegisterUncreatableType<SpriteData>("App.SpriteData", 1, 0, "SpriteData", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WarpData>("App.WarpData", 1, 0, "WarpData", "Can't instantiate in QML");

  qmlRegisterUncreatableType<Player>("App.Player", 1, 0, "Player", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PlayerBasics>("App.PlayerBasics", 1, 0, "PlayerBasics", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PlayerPokedex>("App.PlayerPokedex", 1, 0, "PlayerPokedex", "Can't instantiate in QML");
  qmlRegisterUncreatableType<PlayerPokemon>("App.PlayerPokemon", 1, 0, "PlayerPokemon", "Can't instantiate in QML");

  qmlRegisterUncreatableType<World>("App.World", 1, 0, "World", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldCompleted>("App.WorldCompleted", 1, 0, "WorldCompleted", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldEvents>("App.WorldEvents", 1, 0, "WorldEvents", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Options>("App.Options", 1, 0, "Options", "Can't instantiate in QML");
  qmlRegisterUncreatableType<LetterDelay>("App.LetterDelay", 1, 0, "LetterDelay", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldGeneral>("App.WorldGeneral", 1, 0, "WorldGeneral", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldHidden>("App.WorldHidden", 1, 0, "WorldHidden", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldMissables>("App.WorldMissables", 1, 0, "WorldMissables", "Can't instantiate in QML");
  qmlRegisterUncreatableType<Playtime>("App.Playtime", 1, 0, "Playtime", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldOther>("App.WorldOther", 1, 0, "WorldOther", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldScripts>("App.WorldScripts", 1, 0, "WorldScripts", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldTowns>("App.WorldTowns", 1, 0, "WorldTowns", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldTrades>("App.WorldTrades", 1, 0, "WorldTrades", "Can't instantiate in QML");
  qmlRegisterUncreatableType<WorldLocal>("App.WorldLocal", 1, 0, "WorldLocal", "Can't instantiate in QML");
}
