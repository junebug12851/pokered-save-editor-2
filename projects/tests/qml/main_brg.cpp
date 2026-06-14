/*
  * Copyright 2026 Twilight
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

/**
 * @file main_brg.cpp
 * @brief Qt Quick Test harness that boots a real Bridge over the BaseSAV fixture
 *        and exposes it to QML as `brg` -- so the .qml cases can drive the actual
 *        C++<->QML property chain from QML's side. This guards the "undefined
 *        chain" regression (session 13): if a pointer in `brg.file.data.dataExpanded
 *        .*` weren't registered/traversable, QML would read it as `undefined`.
 */

#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QObject>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldother.h>

#include <bridge/bridge.h>
#include <bridge/router.h>

using namespace pse_test;

class BrgSetup : public QObject
{
  Q_OBJECT

  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;

public slots:
  void qmlEngineAvailable(QQmlEngine* engine)
  {
    DB::inst();             // boot the game databases
    Router::loadScreens();  // the bridge's router needs its screen registry

    // Register the pointer types that QML traverses through the `brg.*` chain so
    // they don't read back as `undefined` (the historic bug class).
    qRegisterMetaType<FileManagement*>("FileManagement*");
    qRegisterMetaType<SaveFile*>("SaveFile*");
    qRegisterMetaType<SaveFileExpanded*>("SaveFileExpanded*");
    qRegisterMetaType<Player*>("Player*");
    qRegisterMetaType<PlayerBasics*>("PlayerBasics*");
    qRegisterMetaType<World*>("World*");
    qRegisterMetaType<WorldOther*>("WorldOther*");

    m_file = new FileManagement;
    loadInto(*m_file->data, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
    m_brg = new Bridge(m_file);

    engine->rootContext()->setContextProperty(QStringLiteral("brg"), m_brg);
  }
};

QUICK_TEST_MAIN_WITH_SETUP(pse_qml_brg, BrgSetup)
#include "main_brg.moc"
