/*
  * Copyright 2019 Twilight
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

#include <QApplication>
#include <QIcon>
#include <QElapsedTimer>
#include <QDebug>

#include "../../ui/window/mainwindow.h"
#include "../bridge/router.h"

extern void bootDatabase();
extern void bootQmlLinkage();

// There is only ever one main window.
MainWindow* mainWindow = nullptr;

// Sets critical Qt options that must be configured before QApplication is created.
static void preBootAttributes()
{
  // Qt 6: High-DPI scaling is enabled automatically — no setAttribute needed.
  // Qt 6: AA_CompressHighFrequencyEvents and AA_CompressTabletEvents are on by default.
  // Qt 6: AA_DisableWindowContextHelpButton is the default on all platforms.
  // Nothing to set here — kept as a hook for future platform-specific tweaks.
}

// Creates and configures the QApplication instance.
[[nodiscard]] static QApplication* createApp(int argc, char* argv[])
{
  preBootAttributes();

  QApplication::setApplicationName("Pokered Save Editor");
  QApplication::setOrganizationName("Twilight");
  QApplication::setApplicationVersion("v1.0.0");
  QApplication::setOrganizationDomain("pokeredsaveeditor.twilight.app");

  // Note: Do NOT set a custom QSurfaceFormat with MSAA here.
  // QQuickWidget renders into an offscreen FBO; requesting MSAA via
  // setDefaultFormat() causes the scene graph to hang on context creation
  // on many Windows drivers. Qt Quick handles its own antialiasing internally.

  auto* app = new QApplication(argc, argv);

  qSetMessagePattern("[%{type}]: %{message} ~ %{time} %{file} %{function} %{line}");

  app->setWindowIcon(QIcon("qrc:/assets/icons/app/512x512.png"));

  // Qt 6 uses QRandomGenerator internally; no manual seeding required.

  mainWindow = new MainWindow();
  mainWindow->show();

  return app;
}

// Performs full program bootstrapping and returns a re