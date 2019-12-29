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

#include "./data/eventpokemon.h"
#include "./data/events.h"
#include "./data/fly.h"
#include "./data/fonts.h"
#include "./data/hiddenCoins.h"
#include "./data/hiddenItems.h"
#include "./data/items.h"
#include "./data/maps.h"
#include "./data/missables.h"
#include "./data/moves.h"
#include "./data/names.h"
#include "./data/pokemon.h"
#include "./data/scripts.h"
#include "./data/sprites.h"
#include "./data/starterPokemon.h"
#include "./data/tmHm.h"
#include "./data/trades.h"
#include "./data/trainers.h"
#include "./data/types.h"

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

  // Pull the icon from resources and set as window icon
  // It's also set to properly be built-in during compile
  QIcon icon("qrc:/assets/icons/512x512.png");
  app->setWindowIcon(icon);

  // Seed random generator
  qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

  // Stil being implemented
  //MainWindow win;
  //win.show();

  return app;
}

// Step 1: Load all JSON data into memory, properly parsed
void load()
{
  EventPokemon::load();
  Events::load();
  Fly::load();
  Font::load();
  HiddenCoins::load();
  HiddenItems::load();
  Items::load();
  Maps::load();
  Missables::load();
  Moves::load();
  Names::load();
  Pokemon::load();
  Scripts::load();
  Sprites::load();
  StarterPokemon::load();
  TmHms::load();
  Trades::load();
  Trainers::load();
  Types::load();
}

// Step 2: Index most JSON data in memory making it rapidly accessible
// not everything can be indexed.
void index()
{
  Events::index();
  Fly::index();
  Font::index();
  Items::index();
  Maps::index();
}

// Performs program one-time bootstrapping and setup
extern QApplication* boot(int argc, char *argv[])
{
  auto app = preBoot(argc, argv);

  load();
  index();

  return app;
}
