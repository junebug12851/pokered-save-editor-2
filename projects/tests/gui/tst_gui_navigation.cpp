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
 * @file tst_gui_navigation.cpp
 * @brief GUI navigation sweep. Boots the REAL app shell (App.qml + AppWindow +
 *        the live C++ Router driving both StackViews) headless, then navigates to
 *        every registered screen the way the UI buttons do (brg.router.changeScreen)
 *        and asserts the screen actually pushes/instantiates with ZERO QML
 *        warnings/errors -- on BOTH a populated save and a fresh New File.
 *
 * This is the "full navigation sweep" half of the comprehensive GUI plan: it proves
 * every screen opens and renders its bound data through the real navigation path,
 * not in isolation (unlike the load-only tst_qml_screens smoke test). Detail screens
 * that require a parent selection (pokemonDetails, mapDetails -- their Screen.homeBtn
 * is false) are excluded here and covered by a targeted parent->select->detail flow
 * (see notes/plans/testing.md -> "Broader GUI coverage").
 */

#include <QtTest>

#include "../helpers/guiapp.h"

#include <pse-db/db.h>
#include <bridge/router.h>

using namespace pse_test;

class TestGuiNavigation : public QObject
{
  Q_OBJECT

  GuiApp* m_app = nullptr;

  // Reset to the Home baseline (non-modal stack at Home, no modal open).
  void toHomeBaseline()
  {
    // Pop anything above Home.
    for (int guard = 0; guard < 12 && m_app->routerStackDepth() > 1; ++guard)
      m_app->closeTop();
  }

  // Drive a populated or fresh app through every non-detail screen.
  void sweep(const QString& fixture)
  {
    GuiApp app(fixture);
    m_app = &app;
    QVERIFY2(app.start(), qPrintable("App.qml failed to load (" + fixture + ")"));

    // App.qml opens the New File modal on startup; dismiss it to reach Home.
    app.closeTop();
    QCOMPARE(app.routerTitle(), QStringLiteral("Home"));

    // Accumulate every screen's problems and report them together at the end, so one
    // run names ALL bad screens (not just the first) -- far easier to triage a sweep.
    QStringList problems;

    const QList<QString> names = Router::screens.keys();
    for (const QString& name : names) {
      Screen* s = Router::screens.value(name, nullptr);
      if (!s || s->url.isEmpty())   continue;   // the empty "" fallback screen
      if (!s->homeBtn)              continue;   // detail screens need a selection context

      const QObject* rootObj = app.view()->rootObject();
      const int rootDepthBefore = rootObj ? rootObj->property("depth").toInt() : -1;

      QmlWarningScope scope;          // any QML warning during this transition is a problem
      app.navigate(name);

      if (!scope.clean())
        problems << QStringLiteral("[%1] navigate(%2) emitted QML warnings:\n  %3")
                      .arg(fixture, name, scope.messages().join(QStringLiteral("\n  ")));

      if (s->modal) {
        // Modal screens push onto the outer stack (appRoot); the title is left
        // unchanged by design (Router only sets title for non-modal screens).
        const int rootDepthAfter = rootObj ? rootObj->property("depth").toInt() : -1;
        if (rootDepthAfter <= rootDepthBefore)
          problems << QStringLiteral("[%1] modal %2 did not push onto the shell stack")
                        .arg(fixture, name);
      } else {
        // Non-modal screens become the current page of the inner stack and set the title.
        if (app.routerTitle() != s->title)
          problems << QStringLiteral("[%1] non-modal %2 title is '%3', expected '%4'")
                        .arg(fixture, name, app.routerTitle(), s->title);
        if (app.currentNonModal() == nullptr)
          problems << QStringLiteral("[%1] non-modal %2 left no current page").arg(fixture, name);
      }

      toHomeBaseline();               // return to a known state for the next screen
    }

    m_app = nullptr;
    QVERIFY2(problems.isEmpty(), qPrintable("\n" + problems.join(QLatin1Char('\n'))));
  }

private slots:
  void initTestCase();
  void navigatesEveryScreen_populatedSave();
  void navigatesEveryScreen_newFile();
  void homeUnwind_clean();
};

void TestGuiNavigation::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  GuiApp::installMessageHandler();
}

// Every screen opens cleanly over a realistic, progressed save.
void TestGuiNavigation::navigatesEveryScreen_populatedSave()
{
  // Fixture must exist (the populated path matters: screens bind real party/dex/items).
  QCOMPARE(readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")).size(), kSaveSize);
  sweep(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
}

// Every screen opens cleanly over a blank New File (empty party/boxes/dex --
// the null/empty-state path that often hides binding bugs).
void TestGuiNavigation::navigatesEveryScreen_newFile()
{
  sweep(QStringLiteral("new"));
}

// goHome unwinds the stack back to Home without emitting warnings.
void TestGuiNavigation::homeUnwind_clean()
{
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();                       // dismiss the startup New File modal

  app.navigate(QStringLiteral("bag"));
  app.navigate(QStringLiteral("pokemon"));

  QmlWarningScope scope;
  app.navigate(QStringLiteral("home"));
  QVERIFY2(scope.clean(), qPrintable("home unwind emitted QML warnings:\n" + scope.messages().join('\n')));
  QCOMPARE(app.routerTitle(), QStringLiteral("Home"));
}

QTEST_MAIN(TestGuiNavigation)
#include "tst_gui_navigation.moc"
