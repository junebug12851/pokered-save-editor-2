/*
  * Copyright 2019 June Hanabi
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

#include <QtCore/qglobal.h>
#include <QApplication>
#include <QIcon>
#include <QDateTime>

#include "./data/db/eventpokemon.h"
#include "./data/db/events.h"
#include "./data/db/fly.h"
#include "./data/db/fonts.h"
#include "./data/db/hiddenCoins.h"
#include "./data/db/hiddenItems.h"
#include "./data/db/items.h"
#include "./data/db/maps.h"
#include "./data/db/missables.h"
#include "./data/db/moves.h"
#include "./data/db/music.h"
#include "./data/db/names.h"
#include "./data/db/namesPokemon.h"
#include "./data/db/pokemon.h"
#include "./data/db/scripts.h"
#include "./data/db/sprites.h"
#include "./data/db/spriteSet.h"
#include "./data/db/starterPokemon.h"
#include "./data/db/tileset.h"
#include "./data/db/tmHm.h"
#include "./data/db/trades.h"
#include "./data/db/trainers.h"
#include "./data/db/types.h"

#include "../ui/window/mainwindow.h"

#include "./data/file/expanded/savefileexpanded.h"
#include "./data/file/filemanagement.h"
#include "./data/file/savefile.h"
#include "./data/file/savefiletoolset.h"

#ifdef QT_DEBUG
#include <QtDebug>
#endif

MainWindow* mainWindow;

// Does a pre-boot of the app setting critical options that have to be done
// before everything else
QApplication* preBoot(int argc, char *argv[])
{
  // Set Attributes
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
  QApplication::setAttribute(Qt::AA_CompressTabletEvents);
  QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

  // Set Application Info
  QApplication::setApplicationName("Pokered Save Editor");
  QApplication::setOrganizationName("June Hanabi");
  QApplication::setApplicationVersion("v1.0.0");
  QApplication::setOrganizationDomain("pokeredsaveeditor.junehanabi.gmail.com");

  // Create the app
  QApplication* app = new QApplication(argc, argv);

  // Setup debug/error messages
  qSetMessagePattern("[%{type}]: %{message} ~ %{time} %{file} %{function} %{line}");

  // Pull the icon from resources and set as window icon
  // It's also set to properly be built-in during compile
  QIcon icon("qrc:/assets/icons/512x512.png");
  app->setWindowIcon(icon);

  // Seed random generator
  qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

  // Stil being implemented
  mainWindow = new MainWindow();
  mainWindow->show();

  return app;
}

// Step 1: Load all JSON data into memory, properly parsed
void load()
{
  EventPokemonDB::load();
  EventsDB::load();
  FlyDB::load();
  FontsDB::load();
  HiddenCoinsDB::load();
  HiddenItemsDB::load();
  ItemsDB::load();
  MapsDB::load();
  MissablesDB::load();
  MovesDB::load();
  MusicDB::load();
  NamesDB::load();
  NamesPokemonDB::load();
  PokemonDB::load();
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
  MapsDB::deepLink();
  MissablesDB::deepLink();
  MovesDB::deepLink();
  PokemonDB::deepLink(); // <-- Definately the most expensive operation!!!
  SpriteSetDB::deepLink();
  StarterPokemonDB::deepLink();
  TmHmsDB::deepLink();
  TradesDB::deepLink();
}

// Performs program one-time bootstrapping and setup
extern QApplication* boot(int argc, char *argv[])
{
  auto app = preBoot(argc, argv);

  load();
  index();
  deepLink();

  // Open recent file
  //auto file = mainWindow->file;
  //file->openFileRecent(0);
  //file->reopenFile();

  //auto data = file->data;
  //auto expanded = data->dataExpanded;
  //expanded->randomize();

  //data->flattenData();
  //file->saveFile();

  return app;
}
