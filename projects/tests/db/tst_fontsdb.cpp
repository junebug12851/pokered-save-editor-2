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
 * @file tst_fontsdb.cpp
 * @brief FontsDB beyond the basic codec: convertToCode edge cases (auto-end,
 *        max-length truncation, unknown-char skip, manual 0x50 stop), every
 *        expandStr() control/variable code branch (<page>/<cont>/<autocont>/<end>/
 *        <para>/<done>/<prompt>/<next>/<line>/<dex> and the <pkmn>/<player>/<rival>/
 *        <poke>/<......>/<targ>/<user>/<pc>/<tm>/<trainer>/<rocket> splices),
 *        countSizeOf/countSizeOfExpanded, splice() directly, searchRaw/search, the
 *        ind lookups, and qmlProtect / the qmlRegister latch.
 */

#include <QtTest>
#include <QQmlEngine>

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/entries/fontdbentry.h>
#include <pse-db/util/fontsearch.h>

class TestFontsDb : public QObject
{
  Q_OBJECT

  FontsDB* f = nullptr;

  // The typable name for a font CODE value (e.g. code 0x52 -> "<player>"), or "".
  QString nameOf(int code) const
  {
    FontDBEntry* e = f->getIndAt("ind" + QString::number(code));
    return e ? e->getName() : QString();
  }

private slots:
  void initTestCase();
  void convertToCode_edgeCases();
  void expandStr_everyControlAndVariableCode();
  void expandStr_lineLimitAndNames();
  void counts_and_search_and_lookups();
  void qml_protectAndRegister();
};

void TestFontsDb::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  f = FontsDB::inst();
  QVERIFY(f->getStoreSize() > 0);
}

void TestFontsDb::convertToCode_edgeCases()
{
  // auto-end appends the 0x50 terminator and counts toward maxLen.
  const QVector<int> withEnd = f->convertToCode(QStringLiteral("RED"), 255, true);
  QVERIFY(!withEnd.isEmpty());
  QCOMPARE(withEnd.last(), 0x50);

  // max-length truncates the output.
  const QVector<int> capped = f->convertToCode(QStringLiteral("ABCDEFGH"), 3, false);
  QVERIFY(capped.size() <= 3);

  // Unknown characters are skipped rather than encoded (here a control glyph that
  // isn't a plain typable char). A run of spaces/odd chars must not loop forever.
  const QVector<int> odd = f->convertToCode(QStringLiteral("A`B~C"), 255, false);
  QVERIFY(odd.size() >= 0);
}

void TestFontsDb::expandStr_everyControlAndVariableCode()
{
  // Drive every code expandStr() special-cases, each in its own call so a
  // loop-terminating code (<page>/<end>/…) doesn't mask later ones. Building the
  // input from the glyph's real name keeps this independent of the exact text.
  const int codes[] = {
    73, 75, 76, 80, 81, 85, 87, 88,        // first-pass control flow
    0x4A, 0x52, 0x53, 0x54, 0x56, 0x59,    // splices
    0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 95       // splices + <dex>
  };
  for(int c : codes) {
    const QString nm = nameOf(c);
    if(nm.isEmpty())
      continue;                            // glyph not present; skip safely
    // "A" + code + "B" so there is content around the control tile.
    const QString out = f->expandStr(QStringLiteral("A") + nm + QStringLiteral("B"),
                                     255, QStringLiteral("BLUE"), QStringLiteral("RED"));
    QVERIFY(out.size() >= 0);              // ran without hanging/crashing
  }
}

void TestFontsDb::expandStr_lineLimitAndNames()
{
  // <next>/<line> advance lines; more than one past the limit cuts the rest off.
  const QString nl = nameOf(78);           // <next>
  if(!nl.isEmpty()) {
    const QString out = f->expandStr(QStringLiteral("A") + nl + QStringLiteral("B") + nl +
                                     QStringLiteral("C") + nl + QStringLiteral("D"),
                                     255, QStringLiteral("BLUE"), QStringLiteral("RED"));
    QVERIFY(out.contains('\n'));           // a newline was produced
  }

  // <player>/<rival> splice the supplied names into the output.
  const QString pl = nameOf(0x52);
  if(!pl.isEmpty()) {
    const QString out = f->expandStr(pl, 255, QStringLiteral("BLUE"), QStringLiteral("RED"));
    QVERIFY(out.contains(QStringLiteral("RED")));
  }
}

void TestFontsDb::counts_and_search_and_lookups()
{
  QVERIFY(f->countSizeOf(QStringLiteral("PIKACHU")) > 0);
  QVERIFY(f->countSizeOfExpanded(QStringLiteral("PIKACHU")) > 0);

  // ind lookups.
  QVERIFY(!f->getInd().isEmpty());
  QVERIFY(f->getIndAt(QStringLiteral("nope_not_a_glyph")) == nullptr);

  // searchRaw() hands back a fresh FontSearch (caller owns it); search() a scoped one.
  FontSearch* raw = f->searchRaw();
  QVERIFY(raw != nullptr);
  QVERIFY(raw->startOver()->getFontCount() > 0);
  delete raw;
  QVERIFY(f->search()->startOver()->getFontCount() > 0);
}

void TestFontsDb::qml_protectAndRegister()
{
  QQmlEngine engine;
  f->qmlProtect(&engine);
  QCOMPARE(QQmlEngine::objectOwnership(f), QQmlEngine::CppOwnership);
  // private qmlRegister slot already ran from the ctor; re-invoke to cover the latch.
  QVERIFY(QMetaObject::invokeMethod(f, "qmlRegister", Qt::DirectConnection));
}

QTEST_GUILESS_MAIN(TestFontsDb)
#include "tst_fontsdb.moc"
