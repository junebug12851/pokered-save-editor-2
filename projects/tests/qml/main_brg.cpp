/*
  * Copyright 2026 Fairy Fox
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

/// The exe's QML type registration (app/src/boot/bootQmlLinkage.cpp, compiled into this target).
extern void bootQmlLinkage();

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

    // The EXE's own registration, not a copy of it (2026-07-12). This used to hand-roll a short
    // list of qRegisterMetaType calls, which meant the test could be green while the real app
    // shipped a type QML could not touch -- exactly what happened to WarpData*/SignData*/
    // SpriteData* (area.warps.warpAt(0) threw "Unknown method return type" in the running app).
    // One source of truth: call what main() calls.
    bootQmlLinkage();

    m_file = new FileManagement;
    loadInto(*m_file->data, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
    m_brg = new Bridge(m_file);

    engine->rootContext()->setContextProperty(QStringLiteral("brg"), m_brg);
  }
};

QUICK_TEST_MAIN_WITH_SETUP(pse_qml_brg, BrgSetup)
#include "main_brg.moc"
