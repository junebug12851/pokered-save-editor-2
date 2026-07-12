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
#pragma once
#include <QObject>

#include "./settings.h"
#include "./router.h"

#include <pse-common/utility.h>

#include <pse-savefile/filemanagement.h>

#include "../mvc/recentfilesmodel.h"
#include "../mvc/creditsmodel.h"
#include "../mvc/fontkeyboard.h"
#include "../mvc/pokemonstartersmodel.h"
#include "../mvc/pokedexmodel.h"
#include "../mvc/itemselectmodel.h"
#include "../mvc/itemstoragemodel.h"
#include "../mvc/itemoverviewmodel.h"
#include "../mvc/itemexchangemodel.h"
#include "../mvc/pokemonoverviewmodel.h"
#include "../mvc/itemmarketmodel.h"
#include "../mvc/itemmarketcartmodel.h"
#include "../mvc/itemmarketviewmodel.h"
#include "../mvc/pokemonstoragemodel.h"
#include "../mvc/pokemonboxselectmodel.h"
#include "../mvc/typesmodel.h"
#include "../mvc/speciesselectmodel.h"
#include "../mvc/statusselectmodel.h"
#include "../mvc/natureselectmodel.h"
#include "../mvc/moveselectmodel.h"
#include "../mvc/mapmodel.h"
#include "../engine/musicplayer.h"

#include <pse-db/fontsdb.h>
#include <pse-db/music.h>
#include <pse-db/names.h>
#include <pse-db/entries/namespokemon.h>
#include <pse-db/entries/examplesplayer.h>
#include <pse-db/entries/examplespokemon.h>
#include <pse-db/entries/examplesrival.h>

/**
 * @brief The single QML<->C++ doorway -- everything the UI touches hangs off here.
 *
 * Injected into the QML context as `brg`, this object aggregates, as Q_PROPERTYs:
 * the live save (@ref file -> the `data.dataExpanded.*` chain QML edits), the
 * @ref router and @ref settings, the shared databases/randomizers QML uses
 * directly (fonts, names, examples, util), and the fleet of Qt item-models
 * (@ref mvc) that adapt C++ data into list/table form for QML views.
 *
 * It is created in MainWindow and given the FileManagement; the DB-backed members
 * are singletons (`*::inst()`), the models are owned here. Registered uncreatable
 * with QML. This is the hub of the app's [bridge pattern](../../../../notes/systems/app.md).
 *
 * @see FileManagement (the save), Router, Settings, and the `mvc/` models.
 */
class Bridge : public QObject
{
  Q_OBJECT

  Q_PROPERTY(FileManagement* file MEMBER file NOTIFY fileChanged) ///< The live save (open/save + the editable tree).
  Q_PROPERTY(RecentFilesModel* recentFilesModel MEMBER recentFilesModel NOTIFY recentFilesModelChanged) ///< Recent-files list model.
  Q_PROPERTY(PokedexModel* pokedexModel MEMBER pokedexModel NOTIFY pokedexModelChanged) ///< Pokedex grid model.
  Q_PROPERTY(Router* router MEMBER router NOTIFY routerChanged) ///< Screen navigation.
  Q_PROPERTY(CreditsModel* creditsModel MEMBER creditsModel NOTIFY creditsModelChanged) ///< Credits list model.
  Q_PROPERTY(PokemonStartersModel* starterModel MEMBER starterModel NOTIFY starterModelChanged) ///< Starter picker model.
  Q_PROPERTY(ItemSelectModel* itemSelectModel MEMBER itemSelectModel NOTIFY itemSelectModelChanged) ///< Item picker model.
  Q_PROPERTY(ItemStorageModel* bagItemsModel MEMBER bagItemsModel NOTIFY bagItemsModelChanged) ///< Bag items model.
  Q_PROPERTY(ItemStorageModel* pcItemsModel MEMBER pcItemsModel NOTIFY pcItemsModelChanged) ///< PC items model.
  Q_PROPERTY(ItemOverviewModel* itemOverviewModel MEMBER itemOverviewModel NOTIFY itemOverviewModelChanged) ///< Bag+storage "where are my items" overview (View All pane).
  Q_PROPERTY(PokemonOverviewModel* pokemonOverviewModel MEMBER pokemonOverviewModel NOTIFY pokemonOverviewModelChanged) ///< Party+boxes "where are my Pokemon" overview (View All pane).
  Q_PROPERTY(FontKeyboard* keyboard MEMBER keyboard NOTIFY keyboardChanged) ///< The full keyboard's tile->key map.
  Q_PROPERTY(FontsDB* fonts MEMBER fonts NOTIFY fontsChanged) ///< The font database (codec + glyphs).
  Q_PROPERTY(NamesPlayer* randomPlayerName MEMBER randomPlayerName NOTIFY randomPlayerNameChanged) ///< Random player-name source.
  Q_PROPERTY(NamesPokemon* randomPokemonName MEMBER randomPokemonName NOTIFY randomPokemonNameChanged) ///< Random nickname source.
  Q_PROPERTY(Utility* util MEMBER util NOTIFY utilChanged) ///< Common utility helpers.
  Q_PROPERTY(ExamplesPlayer* randomExamplePlayer MEMBER randomExamplePlayer NOTIFY randomExamplePlayerChanged) ///< Example-player source.
  Q_PROPERTY(ExamplesPokemon* randomExamplePokemon MEMBER randomExamplePokemon NOTIFY randomExamplePokemonChanged) ///< Example-pokemon source.
  Q_PROPERTY(ExamplesRival* randomExampleRival MEMBER randomExampleRival NOTIFY randomExampleRivalChanged) ///< Example-rival source.
  Q_PROPERTY(Settings* settings MEMBER settings NOTIFY settingsChanged) ///< UI theme/layout settings.
  Q_PROPERTY(ItemExchangeModel* itemExchangeModel MEMBER itemExchangeModel NOTIFY itemExchangeModelChanged) ///< Item<->item exchange (Market Exchange tab: Healing/Custom).
  Q_PROPERTY(ItemMarketModel* marketModel MEMBER marketModel NOTIFY marketModelChanged) ///< Poke-mart market model.
  Q_PROPERTY(ItemMarketCartModel* marketCartModel MEMBER marketCartModel NOTIFY marketCartModelChanged) ///< Cart-only filter of @ref marketModel (receipt pane).
  Q_PROPERTY(ItemMarketViewModel* marketViewModel MEMBER marketViewModel NOTIFY marketViewModelChanged) ///< Buy/Sell view filter of @ref marketModel (left list).
  Q_PROPERTY(PokemonStorageModel* pokemonStorageModel1 MEMBER pokemonStorageModel1 NOTIFY pokemonStorageModel1Changed) ///< PC box-set 1 model.
  Q_PROPERTY(PokemonStorageModel* pokemonStorageModel2 MEMBER pokemonStorageModel2 NOTIFY pokemonStorageModel2Changed) ///< PC box-set 2 model.
  Q_PROPERTY(PokemonBoxSelectModel* pokemonBoxSelectModel1 MEMBER pokemonBoxSelectModel1 NOTIFY pokemonBoxSelectModel1Changed) ///< Box selector 1 model.
  Q_PROPERTY(PokemonBoxSelectModel* pokemonBoxSelectModel2 MEMBER pokemonBoxSelectModel2 NOTIFY pokemonBoxSelectModel2Changed) ///< Box selector 2 model.
  Q_PROPERTY(TypesModel* typesModel MEMBER typesModel NOTIFY typesModelChanged) ///< Types list model.
  Q_PROPERTY(SpeciesSelectModel* speciesSelectModel MEMBER speciesSelectModel NOTIFY speciesSelectModelChanged) ///< Species picker model.
  Q_PROPERTY(StatusSelectModel* statusSelectModel MEMBER statusSelectModel NOTIFY statusSelectModelChanged) ///< Status picker model.
  Q_PROPERTY(NatureSelectModel* natureSelectModel MEMBER natureSelectModel NOTIFY natureSelectModelChanged) ///< Nature picker model.
  Q_PROPERTY(MoveSelectModel* moveSelectModel MEMBER moveSelectModel NOTIFY moveSelectModelChanged) ///< Move picker model.
  Q_PROPERTY(MapModel* map MEMBER map NOTIFY mapChanged) ///< The loaded map: its rendered image + the game's view boxes.
  Q_PROPERTY(MusicPlayer* music MEMBER music NOTIFY musicChanged) ///< The Game Boy's sound, live (play / preview / select).
  Q_PROPERTY(MusicDB* musicDb MEMBER musicDb NOTIFY musicDbChanged) ///< The 46 real tracks (name/bank/id).

signals:
  void fileChanged();
  void recentFilesModelChanged();
  void pokedexModelChanged();
  void routerChanged();
  void creditsModelChanged();
  void keyboardChanged();
  void fontsChanged();
  void randomPlayerNameChanged();
  void randomPokemonNameChanged();
  void utilChanged();
  void randomExamplePlayerChanged();
  void randomExamplePokemonChanged();
  void randomExampleRivalChanged();
  void settingsChanged();
  void starterModelChanged();
  void itemSelectModelChanged();
  void bagItemsModelChanged();
  void pcItemsModelChanged();
  void itemOverviewModelChanged();
  void itemExchangeModelChanged();
  void pokemonOverviewModelChanged();
  void marketModelChanged();
  void marketCartModelChanged();
  void marketViewModelChanged();
  void pokemonStorageModel1Changed();
  void pokemonStorageModel2Changed();
  void pokemonBoxSelectModel1Changed();
  void pokemonBoxSelectModel2Changed();
  void typesModelChanged();
  void speciesSelectModelChanged();
  void statusSelectModelChanged();
  void natureSelectModelChanged();
  void moveSelectModelChanged();
  void mapChanged();
  void musicChanged();
  void musicDbChanged();

public:
  /// @param file the live FileManagement to expose as `brg.file`.
  Bridge(FileManagement* file);

  FileManagement* file = nullptr; ///< @see file property.

  FontKeyboard* keyboard = new FontKeyboard;                     ///< @see keyboard property.

  Router* router = new Router;                                   ///< @see router property.
  FontsDB* fonts = FontsDB::inst();                              ///< @see fonts property.
  NamesPlayer* randomPlayerName = NamesPlayer::inst();           ///< @see randomPlayerName property.
  NamesPokemon* randomPokemonName = NamesPokemon::inst();        ///< @see randomPokemonName property.
  Utility* util = Utility::inst();                               ///< @see util property.
  ExamplesPlayer* randomExamplePlayer = ExamplesPlayer::inst();  ///< @see randomExamplePlayer property.
  ExamplesPokemon* randomExamplePokemon = ExamplesPokemon::inst(); ///< @see randomExamplePokemon property.
  ExamplesRival* randomExampleRival = ExamplesRival::inst();     ///< @see randomExampleRival property.
  Settings* settings = new Settings(file->data);                ///< @see settings property.

  RecentFilesModel* recentFilesModel = nullptr;       ///< @see recentFilesModel property.
  PokedexModel* pokedexModel = nullptr;               ///< @see pokedexModel property.
  ItemStorageModel* bagItemsModel = nullptr;          ///< @see bagItemsModel property.
  ItemStorageModel* pcItemsModel = nullptr;           ///< @see pcItemsModel property.
  ItemOverviewModel* itemOverviewModel = nullptr;     ///< @see itemOverviewModel property.
  ItemExchangeModel* itemExchangeModel = nullptr;     ///< @see itemExchangeModel property.
  PokemonOverviewModel* pokemonOverviewModel = nullptr; ///< @see pokemonOverviewModel property.
  ItemMarketModel* marketModel = nullptr;             ///< @see marketModel property.
  ItemMarketCartModel* marketCartModel = nullptr;     ///< @see marketCartModel property.
  ItemMarketViewModel* marketViewModel = nullptr;     ///< @see marketViewModel property.
  PokemonStorageModel* pokemonStorageModel1 = nullptr; ///< @see pokemonStorageModel1 property.
  PokemonStorageModel* pokemonStorageModel2 = nullptr; ///< @see pokemonStorageModel2 property.
  PokemonBoxSelectModel* pokemonBoxSelectModel1 = nullptr; ///< @see pokemonBoxSelectModel1 property.
  PokemonBoxSelectModel* pokemonBoxSelectModel2 = nullptr; ///< @see pokemonBoxSelectModel2 property.
  CreditsModel* creditsModel = new CreditsModel;      ///< @see creditsModel property.
  PokemonStartersModel* starterModel = new PokemonStartersModel; ///< @see starterModel property.
  ItemSelectModel* itemSelectModel = new ItemSelectModel; ///< @see itemSelectModel property.
  TypesModel* typesModel = new TypesModel;            ///< @see typesModel property.
  SpeciesSelectModel* speciesSelectModel = new SpeciesSelectModel; ///< @see speciesSelectModel property.
  StatusSelectModel* statusSelectModel = new StatusSelectModel; ///< @see statusSelectModel property.
  NatureSelectModel* natureSelectModel = new NatureSelectModel; ///< @see natureSelectModel property.
  MoveSelectModel* moveSelectModel = new MoveSelectModel; ///< @see moveSelectModel property.
  MapModel* map = nullptr;                            ///< @see map property.
  MusicPlayer* music = new MusicPlayer;               ///< @see music property.
  MusicDB* musicDb = MusicDB::inst();                 ///< @see musicDb property.
};
