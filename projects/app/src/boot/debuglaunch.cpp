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
 * @file debuglaunch.cpp
 * @brief DEBUG-ONLY startup automation. Parses developer launch flags and, once the
 *        QML shell is live, loads a save and jumps straight to a screen -- instead of
 *        clicking through the New File modal by hand. Meant for fast manual iteration
 *        and for automation harnesses (launch directly into the screen under test).
 *
 *        Flags (all optional):
 *          --sav <path>     load this .sav on startup
 *          --screen <name>  jump to a registered screen after load (trainerCard,
 *                           pokedex, bag, pokemart, pokemon, rival, maps, home, ...)
 *          --select <spec>  selection for screens that need one -- 'random' or
 *                           'source:id' (NOT yet wired -- reserved for the next step)
 *
 *        Guarded by QT_DEBUG: in release builds applyDebugLaunch() is an empty no-op,
 *        so the flags and this whole mechanism simply don't exist in shipped binaries.
 */

#include <QApplication>

#ifdef QT_DEBUG

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStringList>
#include <QTimer>
#include <QDebug>

#include "../../ui/window/mainwindow.h"
#include <QStringList>
#include "../bridge/bridge.h"
#include "../bridge/router.h"
#include <pse-savefile/filemanagement.h>

void applyDebugLaunch(QApplication* app)
{
  QCommandLineParser parser;
  QCommandLineOption savOpt(QStringList() << "sav" << "save",
    QStringLiteral("DEBUG: load this .sav file on startup."), QStringLiteral("path"));
  QCommandLineOption screenOpt(QStringList() << "screen",
    QStringLiteral("DEBUG: jump to this screen after load "
                   "(trainerCard, pokedex, bag, pokemart, pokemon, rival, maps, home)."),
    QStringLiteral("name"));
  QCommandLineOption selectOpt(QStringList() << "select",
    QStringLiteral("DEBUG: selection for screens that need one -- 'random' or 'source:id'."),
    QStringLiteral("spec"));
  QCommandLineOption shotOpt(QStringList() << "shot",
    QStringLiteral("DEBUG: after navigating, grab the screen to this PNG file."),
    QStringLiteral("path"));
  parser.addOption(savOpt);
  parser.addOption(screenOpt);
  parser.addOption(selectOpt);
  parser.addOption(shotOpt);

  // parse() (not process()): tolerate Qt's own args + unknown flags, never exit.
  parser.parse(app->arguments());

  const QString sav    = parser.value(savOpt);
  const QString screen = parser.value(screenOpt);
  const QString select = parser.value(selectOpt);
  const QString shot   = parser.value(shotOpt);

  if(sav.isEmpty() && screen.isEmpty() && shot.isEmpty())
    return; // nothing requested -> normal startup

  // App.qml seats its startup stack (home + the New File modal) on an event-loop
  // tick, NOT necessarily before ours -- if we navigate too early, that modal lands
  // on top of us. So poll until the startup stack is seated (>= 2 entries), then act
  // exactly once. onCompleted runs atomically w.r.t. the loop, so size>=2 means the
  // modal is already up and closeScreen() below can dismiss it.
  QTimer* timer = new QTimer(app);
  int* tries = new int(0);
  QObject::connect(timer, &QTimer::timeout, app, [timer, tries, sav, screen, select, shot]() {
    Bridge* brg = MainWindow::bridge;
    const bool ready = (brg != nullptr) && Router::stack.size() >= 2;
    if(!ready && ++(*tries) < 200) // wait up to ~10s (50ms x 200)
      return;

    timer->stop();
    timer->deleteLater();
    delete tries;

    if(brg == nullptr) {
      qWarning() << "[debug-launch] Bridge not ready; skipping.";
      return;
    }

    if(!sav.isEmpty()) {
      qInfo() << "[debug-launch] loading save:" << sav;
      brg->file->setPath(sav);   // set the active path (does NOT read the file)
      brg->file->reopenFile();   // read + adopt the data from disk for that path
      qInfo() << "[debug-launch] loaded path=" << brg->file->getPath();
    }

    if(!screen.isEmpty()) {
      if(!Router::screens.contains(screen)) {
        qWarning() << "[debug-launch] unknown screen:" << screen
                   << "-- known:" << Router::screens.keys();
        return;
      }

      // Dismiss the startup New File modal, then navigate to the target screen.
      qInfo() << "[debug-launch] pre-nav stack size=" << Router::stack.size();
      brg->router->closeScreen();
      qInfo() << "[debug-launch] post-close stack size=" << Router::stack.size();
      qInfo() << "[debug-launch] screen:" << screen
              << (select.isEmpty() ? QString() : QStringLiteral("select=") + select);
      if(screen == QStringLiteral("pokemonDetails")) {
        // Needs a selected mon -- open party slot N (default 0; --select party:N).
        const int idx = select.startsWith(QStringLiteral("party:"))
                          ? select.mid(6).toInt() : 0;
        qInfo() << "[debug-launch] pokemonDetails party index" << idx;
        if(MainWindow::getInstance())
          MainWindow::getInstance()->debugOpenPartyDetails(idx);
      } else {
        brg->router->changeScreen(screen);
      }
      qInfo() << "[debug-launch] post-change stack size=" << Router::stack.size()
              << "title=" << brg->router->title;

      // TODO(next step): screens that need a selection (pokemonDetails / mapDetails)
      // should honour --select here -- pick a random owned entity, or a specific
      // 'source:id'. Not yet wired.
    }

    // Optional off-screen screenshot once the target screen has had a moment to
    // render. Grabs the QQuickWidget framebuffer, so it works with the window
    // occluded / unfocused (no need to raise or steal focus).
    if(!shot.isEmpty()) {
      QTimer::singleShot(600, qApp, [shot]() {
        if(auto* mw = MainWindow::getInstance()) {
          const bool ok = mw->saveShot(shot);
          qInfo() << "[debug-launch] shot" << (ok ? "saved:" : "FAILED:") << shot;
        }
      });
    }
  });
  timer->setInterval(50);
  timer->start();
}

#else

void applyDebugLaunch(QApplication*) {}

#endif
