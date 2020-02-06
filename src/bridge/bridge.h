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
#include "../common/utility.h"

#include "../data/file/filemanagement.h"

#include "../mvc/recentfilesmodel.h"
#include "../mvc/creditsmodel.h"

#include "../data/db/fonts.h"
#include "../data/db/fontsearch.h"
#include "../data/db/names.h"
#include "../data/db/namesPokemon.h"
#include "../data/db/examplesplayer.h"
#include "../data/db/examplespokemon.h"
#include "../data/db/examplesrival.h"

class Bridge : public QObject
{
  Q_OBJECT

  Q_PROPERTY(FileManagement* file MEMBER file NOTIFY fileChanged)
  Q_PROPERTY(RecentFilesModel* recentFilesModel MEMBER recentFilesModel NOTIFY recentFilesModelChanged)
  Q_PROPERTY(Router* router MEMBER router NOTIFY routerChanged)
  Q_PROPERTY(CreditsModel* creditsModel MEMBER creditsModel NOTIFY creditsModelChanged)
  Q_PROPERTY(FontSearch* fontSearch MEMBER fontSearch NOTIFY fontSearchChanged)
  Q_PROPERTY(FontsDB* fonts MEMBER fonts NOTIFY fontsChanged)
  Q_PROPERTY(NamesDB* randomPlayerName MEMBER randomPlayerName NOTIFY randomPlayerNameChanged)
  Q_PROPERTY(NamesPokemonDB* randomPokemonName MEMBER randomPokemonName NOTIFY randomPokemonNameChanged)
  Q_PROPERTY(Utility* util MEMBER util NOTIFY utilChanged)
  Q_PROPERTY(ExamplesPlayer* randomExamplePlayer MEMBER randomExamplePlayer NOTIFY randomExamplePlayerChanged)
  Q_PROPERTY(ExamplesPokemon* randomExamplePokemon MEMBER randomExamplePokemon NOTIFY randomExamplePokemonChanged)
  Q_PROPERTY(ExamplesRival* randomExampleRival MEMBER randomExampleRival NOTIFY randomExampleRivalChanged)
  Q_PROPERTY(Settings* settings MEMBER settings NOTIFY settingsChanged)

signals:
  void fileChanged();
  void recentFilesModelChanged();
  void routerChanged();
  void creditsModelChanged();
  void fontSearchChanged();
  void fontsChanged();
  void randomPlayerNameChanged();
  void randomPokemonNameChanged();
  void utilChanged();
  void randomExamplePlayerChanged();
  void randomExamplePokemonChanged();
  void randomExampleRivalChanged();
  void settingsChanged();

public:
  Bridge(FileManagement* file);

  FileManagement* file = nullptr;
  RecentFilesModel* recentFilesModel = nullptr;

  Router* router = new Router;
  CreditsModel* creditsModel = new CreditsModel;
  FontSearch* fontSearch = new FontSearch;
  FontsDB* fonts = new FontsDB;
  NamesDB* randomPlayerName = new NamesDB;
  NamesPokemonDB* randomPokemonName = new NamesPokemonDB;
  Utility* util = new Utility;
  ExamplesPlayer* randomExamplePlayer = new ExamplesPlayer;
  ExamplesPokemon* randomExamplePokemon = new ExamplesPokemon;
  ExamplesRival* randomExampleRival = new ExamplesRival;
  Settings* settings = new Settings;
};

#endif
