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

#include "../../ui/window/mainwindow.h"
#include "../bridge/router.h"

extern void bootDatabase();
extern void bootQmlLinkage();

// Main Window, there is only ever one window
MainWindow* mainWindow = nullptr;

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

  // Set Smoothing
  QSurfaceFormat format;
  format.setSamples(8);
  QSurfaceFormat::setDefaultFormat(format);

  // Create the app
  QApplication* app = new QApplication(argc, argv);

  // Setup debug/error messages
  qSetMessagePattern("[%{type}]: %{message} ~ %{time} %{file} %{function} %{line}");

  // Pull the icon from resources and set as window icon
  // It's also set to properly be built-in during compile
  QIcon icon("qrc:/assets/icons/app/512x512.png");
  app->setWindowIcon(icon);

  // Seed random generator
  qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

  // Stil being implemented
  mainWindow = new MainWindow();
  mainWindow->show();

  return app;
}

// Performs program one-time bootstrapping and setup
extern QApplication* boot(int argc, char *argv[])
{
  // Prepare database
  bootDatabase();

  // Register C++ classes with QML
  bootQmlLinkage();

  // Load Screens into the router
  Router::loadScreens();

  // Pre-boot app
  auto app = preBoot(argc, argv);

  // Return generated and prepared app for creation
  return app;
}
