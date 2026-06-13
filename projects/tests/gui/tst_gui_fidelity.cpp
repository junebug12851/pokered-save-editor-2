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
 * @file tst_gui_fidelity.cpp
 * @brief THE byte-fidelity confidence test: prove that *browsing* the app -- moving
 *        between screens, opening and closing dropdowns/select boxes, clicking into
 *        fields, opening and closing edit popups & drawers -- WITHOUT typing or
 *        committing any change touches ZERO save bytes.
 *
 * This is the automated form of the project's sacred promise (principles.md -> "Save
 * File Integrity Is Sacred"): the editor flips only the exact bytes for an edit and
 * leaves every other byte untouched. Here we make NO edit at all, so the on-disk image
 * must come back byte-for-byte identical no matter how much the user clicks around.
 *
 * Method per fixture: take a NO-INTERACTION baseline (load -> dismiss the New File
 * modal -> flatten -> recalc -> snapshot), then in a fresh session do the same but with
 * a heavy non-destructive interaction sweep in between, flatten, and assert the 32 KB
 * image equals the baseline. On a mismatch it prints every changed offset so a stray
 * write is pinpointed, not just detected.
 *
 * "Non-destructive" means strictly browsing: navigation, dropdown open/close (no
 * selection), field focus/blur (no typing), popup/drawer open/close. It deliberately
 * does NOT press randomize/toggle/stepper/checkout/delete controls -- those are edits
 * and are covered (with their byte deltas asserted) by the editing GUI tests.
 */

#include <QtTest>

#include "../helpers/guiapp.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/savefiletoolset.h>
#include <pse-savefile/expanded/savefileexpanded.h>

#include <bridge/router.h>

using namespace pse_test;

class TestGuiFidelity : public QObject
{
  Q_OBJECT

  // Flatten the live save to its raw 32 KB image (the bytes that would hit disk).
  static QByteArray flattenedImage(GuiApp& app)
  {
    SaveFile* sf = app.file()->data;
    sf->flattenData();
    sf->toolset->recalcChecksums();
    return snapshot(*sf);
  }

  // Open the app to a clean Home (modal dismissed) without any further interaction.
  static void toHome(GuiApp& app)
  {
    QVERIFY(app.start());
    app.closeTop();   // dismiss the startup New File modal
  }

  // Reset the non-modal stack back to Home.
  static void unwindToHome(GuiApp& app)
  {
    for (int g = 0; g < 12 && app.routerStackDepth() > 1; ++g)
      app.closeTop();
  }

  // The non-detail, non-empty screens we browse (detail screens need a selection;
  // they're covered by the detail-flow test).
  static QList<QString> browsableScreens()
  {
    QList<QString> out;
    const QList<QString> names = Router::screens.keys();
    for (const QString& name : names) {
      Screen* s = Router::screens.value(name, nullptr);
      if (!s || s->url.isEmpty() || s->modal || !s->homeBtn)
        continue;
      out << name;
    }
    return out;
  }

  // Assert two 32 KB images are identical; on failure name every changed offset.
  static void assertSameImage(const QByteArray& baseline, const QByteArray& after, const QString& what)
  {
    if (baseline == after)
      return;
    const QVector<int> diffs = diffOffsets(baseline, after);
    QStringList off;
    for (int i = 0; i < diffs.size() && i < 40; ++i)
      off << QStringLiteral("0x%1").arg(diffs[i], 4, 16, QLatin1Char('0'));
    QFAIL(qPrintable(QStringLiteral("%1: %2 byte(s) changed by browsing (expected 0). Offsets: %3%4")
                       .arg(what).arg(diffs.size()).arg(off.join(QStringLiteral(", ")))
                       .arg(diffs.size() > 40 ? QStringLiteral(" ...") : QString())));
  }

private slots:
  void initTestCase();
  void navigatingScreens_changesNoBytes_data();
  void navigatingScreens_changesNoBytes();
  void browsingControls_changesNoBytes_data();
  void browsingControls_changesNoBytes();
};

void TestGuiFidelity::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QCOMPARE(readSaveBytes(QStringLiteral("BaseSAV.sav")).size(), kSaveSize);
  GuiApp::installMessageHandler();
}

void TestGuiFidelity::navigatingScreens_changesNoBytes_data()
{
  QTest::addColumn<QString>("fixture");
  QTest::newRow("progressed") << QStringLiteral("BaseSAV.sav");
  QTest::newRow("new")        << QStringLiteral("new");
}

// Walking through every screen (and opening/closing every modal) writes nothing.
void TestGuiFidelity::navigatingScreens_changesNoBytes()
{
  QFETCH(QString, fixture);

  QByteArray baseline;
  { GuiApp base(fixture); toHome(base); baseline = flattenedImage(base); }

  GuiApp app(fixture);
  toHome(app);

  // Every non-modal screen, then back to Home.
  for (const QString& name : browsableScreens()) {
    app.navigate(name);
    unwindToHome(app);
  }
  // Every modal: open then close.
  const QList<QString> names = Router::screens.keys();
  for (const QString& name : names) {
    Screen* s = Router::screens.value(name, nullptr);
    if (!s || s->url.isEmpty() || !s->modal)
      continue;
    app.navigate(name);
    app.closeTop();
  }

  assertSameImage(baseline, flattenedImage(app), QStringLiteral("[%1] navigation").arg(fixture));
}

void TestGuiFidelity::browsingControls_changesNoBytes_data()
{
  QTest::addColumn<QString>("fixture");
  QTest::newRow("progressed") << QStringLiteral("BaseSAV.sav");
  QTest::newRow("new")        << QStringLiteral("new");
}

// On every screen: open + close every dropdown/select box, and focus + blur every
// editable field -- WITHOUT changing a selection or typing. Writes nothing.
void TestGuiFidelity::browsingControls_changesNoBytes()
{
  QFETCH(QString, fixture);

  QByteArray baseline;
  { GuiApp base(fixture); toHome(base); baseline = flattenedImage(base); }

  GuiApp app(fixture);
  toHome(app);

  int combosOpened = 0, fieldsFocused = 0;

  for (const QString& name : browsableScreens()) {
    app.navigate(name);
    QQuickItem* screen = app.currentNonModal();
    if (!screen) { unwindToHome(app); continue; }

    // Open + close every dropdown / select box (no selection change). The custom
    // Select* controls are ComboBox subtypes whose QML class name is "SelectX_QMLTYPE"
    // (NOT "ComboBox"), so we duck-type: any item exposing a `popup` + `currentIndex`.
    QList<QQuickItem*> combos;
    GuiApp::collectItems(screen, [](QQuickItem* i){
      return i->metaObject()->indexOfProperty("popup") >= 0
          && i->metaObject()->indexOfProperty("currentIndex") >= 0;
    }, combos);
    for (QQuickItem* combo : combos) {
      QObject* popup = combo->property("popup").value<QObject*>();
      if (!popup) continue;
      QMetaObject::invokeMethod(popup, "open");
      app.settle(15);
      QMetaObject::invokeMethod(popup, "close");
      app.settle(10);
      ++combosOpened;
    }

    // Focus + blur every editable text field (no typing). Duck-type on the editable
    // text API (`selectByMouse` + `text`) so DefTextEdit_QMLTYPE etc. are all caught.
    QList<QQuickItem*> fields;
    GuiApp::collectItems(screen, [](QQuickItem* i){
      return i->metaObject()->indexOfProperty("selectByMouse") >= 0
          && i->metaObject()->indexOfProperty("text") >= 0;
    }, fields);
    for (QQuickItem* f : fields) {
      f->forceActiveFocus();
      app.settle(5);
      ++fieldsFocused;
    }
    screen->forceActiveFocus();   // blur the last field (commit-on-blur, unchanged value)
    app.settle(10);

    unwindToHome(app);
  }

  // Guard against a false pass: the browse sweep must actually have exercised controls.
  qInfo("[%s] browsed: %d dropdowns opened/closed, %d fields focused",
        qPrintable(fixture), combosOpened, fieldsFocused);
  QVERIFY2(combosOpened + fieldsFocused > 0,
           "browse sweep found no dropdowns or fields -- the control locator is broken");

  assertSameImage(baseline, flattenedImage(app), QStringLiteral("[%1] browsing controls").arg(fixture));
}

QTEST_MAIN(TestGuiFidelity)
#include "tst_gui_fidelity.moc"
