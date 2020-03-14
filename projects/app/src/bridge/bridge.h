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
#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>

#include "./settings.h"
#include "./router.h"

#include <pse-common/utility.h>

#include <pse-savefile/filemanagement.h>

#include "../mvc/recentfilesmodel.h"
#include "../mvc/creditsmodel.h"
#include "../mvc/fontsearchmodel.h"
#include "../mvc/pokemonstartersmodel.h"
#include "../mvc/pokedexmodel.h"
#include "../mvc/itemselectmodel.h"
#include "../mvc/itemstoragemodel.h"
#include "../mvc/itemmarketmodel.h"
#include "../mvc/pokemonstoragemodel.h"
#include "../mvc/pokemonboxselectmodel.h"
#include "../mvc/typesmodel.h"
#include "../mvc/speciesselectmodel.h"
#include "../mvc/statusselectmodel.h"
#include "../mvc/natureselectmodel.h"
#include "../mvc/moveselectmodel.h"
#include "../mvc/mapselectmodel.h"

#include <pse-db/fonts.h>
#include <pse-db/fontsearch.h>
#include <pse-db/names.h>
#include <pse-db/namesPokemon.h>
#include <pse-db/examplesplayer.h>
#include <pse-db/examplespokemon.h>
#include <pse-db/examplesrival.h>

class Bridge : public QObject
{
  Q_OBJECT

  Q_PROPERTY(FileManagement* file MEMBER file NOTIFY fileChanged)
  Q_PROPERTY(RecentFilesModel* recentFilesModel MEMBER recentFilesModel NOTIFY recentFilesModelChanged)
  Q_PROPERTY(PokedexModel* pokedexModel MEMBER pokedexModel NOTIFY pokedexModelChanged)
  Q_PROPERTY(Router* router MEMBER router NOTIFY routerChanged)
  Q_PROPERTY(CreditsModel* creditsModel MEMBER creditsModel NOTIFY creditsModelChanged)
  Q_PROPERTY(PokemonStartersModel* starterModel MEMBER starterModel NOTIFY starterModelChanged)
  Q_PROPERTY(ItemSelectModel* itemSelectModel MEMBER itemSelectModel NOTIFY itemSelectModelChanged)
  Q_PROPERTY(ItemStorageModel* bagItemsModel MEMBER bagItemsModel NOTIFY bagItemsModelChanged)
  Q_PROPERTY(ItemStorageModel* pcItemsModel MEMBER pcItemsModel NOTIFY pcItemsModelChanged)
  Q_PROPERTY(FontSearch* fontSearch MEMBER fontSearch NOTIFY fontSearchChanged)
  Q_PROPERTY(FontSearchModel* fontSearchModel MEMBER fontSearchModel NOTIFY fontSearchModelChanged)
  Q_PROPERTY(FontsDB* fonts MEMBER fonts NOTIFY fontsChanged)
  Q_PROPERTY(NamesDB* randomPlayerName MEMBER randomPlayerName NOTIFY randomPlayerNameChanged)
  Q_PROPERTY(NamesPokemonDB* randomPokemonName MEMBER randomPokemonName NOTIFY randomPokemonNameChanged)
  Q_PROPERTY(Utility* util MEMBER util NOTIFY utilChanged)
  Q_PROPERTY(ExamplesPlayer* randomExamplePlayer MEMBER randomExamplePlayer NOTIFY randomExamplePlayerChanged)
  Q_PROPERTY(ExamplesPokemon* randomExamplePokemon MEMBER randomExamplePokemon NOTIFY randomExamplePokemonChanged)
  Q_PROPERTY(ExamplesRival* randomExampleRival MEMBER randomExampleRival NOTIFY randomExampleRivalChanged)
  Q_PROPERTY(Settings* settings MEMBER settings NOTIFY settingsChanged)
  Q_PROPERTY(ItemMarketModel* marketModel MEMBER marketModel NOTIFY marketModelChanged)
  Q_PROPERTY(PokemonStorageModel* pokemonStorageModel1 MEMBER pokemonStorageModel1 NOTIFY pokemonStorageModel1Changed)
  Q_PROPERTY(PokemonStorageModel* pokemonStorageModel2 MEMBER pokemonStorageModel2 NOTIFY pokemonStorageModel2Changed)
  Q_PROPERTY(PokemonBoxSelectModel* pokemonBoxSelectModel1 MEMBER pokemonBoxSelectModel1 NOTIFY pokemonBoxSelectModel1Changed)
  Q_PROPERTY(PokemonBoxSelectModel* pokemonBoxSelectModel2 MEMBER pokemonBoxSelectModel2 NOTIFY pokemonBoxSelectModel2Changed)
  Q_PROPERTY(TypesModel* typesModel MEMBER typesModel NOTIFY typesModelChanged)
  Q_PROPERTY(SpeciesSelectModel* speciesSelectModel MEMBER speciesSelectModel NOTIFY speciesSelectModelChanged)
  Q_PROPERTY(StatusSelectModel* statusSelectModel MEMBER statusSelectModel NOTIFY statusSelectModelChanged)
  Q_PROPERTY(NatureSelectModel* natureSelectModel MEMBER natureSelectModel NOTIFY natureSelectModelChanged)
  Q_PROPERTY(MoveSelectModel* moveSelectModel MEMBER moveSelectModel NOTIFY moveSelectModelChanged)
  Q_PROPERTY(MapSelectModel* mapSelectModel MEMBER mapSelectModel NOTIFY mapSelectModelChanged)

signals:
  void fileChanged();
  void recentFilesModelChanged();
  void pokedexModelChanged();
  void routerChanged();
  void creditsModelChanged();
  void fontSearchChanged();
  void fontSearchModelChanged();
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
  void marketModelChanged();
  void pokemonStorageModel1Changed();
  void pokemonStorageModel2Changed();
  void pokemonBoxSelectModel1Changed();
  void pokemonBoxSelectModel2Changed();
  void typesModelChanged();
  void speciesSelectModelChanged();
  void statusSelectModelChanged();
  void natureSelectModelChanged();
  void moveSelectModelChanged();
  void mapSelectModelChanged();

public:
  Bridge(FileManagement* file);

  FileManagement* file = nullptr;

  FontSearch* fontSearch = new FontSearch;
  FontSearchModel* fontSearchModel = new FontSearchModel(fontSearch);

  Router* router = new Router;
  FontsDB* fonts = new FontsDB;
  NamesDB* randomPlayerName = new NamesDB;
  NamesPokemonDB* randomPokemonName = new NamesPokemonDB;
  Utility* util = new Utility;
  ExamplesPlayer* randomExamplePlayer = new ExamplesPlayer;
  ExamplesPokemon* randomExamplePokemon = new ExamplesPokemon;
  ExamplesRival* randomExampleRival = new ExamplesRival;
  Settings* settings = new Settings(file->data);

  RecentFilesModel* recentFilesModel = nullptr;
  PokedexModel* pokedexModel = nullptr;
  ItemStorageModel* bagItemsModel = nullptr;
  ItemStorageModel* pcItemsModel = nullptr;
  ItemMarketModel* marketModel = nullptr;
  PokemonStorageModel* pokemonStorageModel1 = nullptr;
  PokemonStorageModel* pokemonStorageModel2 = nullptr;
  PokemonBoxSelectModel* pokemonBoxSelectModel1 = nullptr;
  PokemonBoxSelectModel* pokemonBoxSelectModel2 = nullptr;
  CreditsModel* creditsModel = new CreditsModel;
  PokemonStartersModel* starterModel = new PokemonStartersModel;
  ItemSelectModel* itemSelectModel = new ItemSelectModel;
  TypesModel* typesModel = new TypesModel;
  SpeciesSelectModel* speciesSelectModel = new SpeciesSelectModel;
  StatusSelectModel* statusSelectModel = new StatusSelectModel;
  NatureSelectModel* natureSelectModel = new NatureSelectModel;
  MoveSelectModel* moveSelectModel = new MoveSelectModel;
  MapSelectModel* mapSelectModel = nullptr;
};

#endif
