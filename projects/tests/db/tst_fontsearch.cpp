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
 * @file tst_fontsearch.cpp
 * @brief Exhaustive sweep of FontSearch's fluent filters not reached by tst_fonts:
 *        keepAnyOf (the keyboard's OR/union filter), every and<Trait>/not<Trait>
 *        predicate (shorthand/normal/control/picture/single/multi/variable),
 *        getFonts/fontAt, and qmlProtect / the qmlRegister second-call latch.
 */

#include <QtTest>
#include <QQmlEngine>
#include <functional>

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/entries/fontdbentry.h>
#include <pse-db/util/fontsearch.h>

class TestFontSearch : public QObject
{
  Q_OBJECT

  FontSearch fs;
  int m_all = 0;

private slots:
  void initTestCase();
  void keepAnyOf_unionAndEmpty();
  void everyAndNotPredicate_inBounds();
  void getFonts_fontAt_qmlProtect();
  void qmlRegister_secondCallIsNoOp();
};

void TestFontSearch::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_all = fs.startOver()->getFontCount();
  QVERIFY(m_all > 0);
}

void TestFontSearch::keepAnyOf_unionAndEmpty()
{
  // No traits enabled -> empty set.
  QCOMPARE(fs.keepAnyOf(false, false, false, false, false, false)->getFontCount(), 0);

  // A single trait is a subset; enabling more never shrinks the union.
  const int normalOnly = fs.keepAnyOf(true, false, false, false, false, false)->getFontCount();
  const int normalOrControl = fs.keepAnyOf(true, true, false, false, false, false)->getFontCount();
  QVERIFY(normalOnly > 0 && normalOnly <= m_all);
  QVERIFY(normalOrControl >= normalOnly);

  // Everything enabled keeps every glyph that has at least one trait (<= all).
  const int any = fs.keepAnyOf(true, true, true, true, true, true)->getFontCount();
  QVERIFY(any > 0 && any <= m_all);
}

void TestFontSearch::everyAndNotPredicate_inBounds()
{
  // Each predicate narrows from the full set to a subset in [0, all] and returns
  // the search. An and<Trait>/not<Trait> pair is disjoint, so their sizes sum to
  // <= all (glyphs that are neither sit out both).
  struct P { std::function<int()> on; std::function<int()> off; };
  QVector<P> pairs = {
    { [&]{ return fs.startOver()->andShorthand()->getFontCount(); },  [&]{ return fs.startOver()->notShorthand()->getFontCount(); } },
    { [&]{ return fs.startOver()->andNormal()->getFontCount(); },     [&]{ return fs.startOver()->notNormal()->getFontCount(); } },
    { [&]{ return fs.startOver()->andControl()->getFontCount(); },    [&]{ return fs.startOver()->notControl()->getFontCount(); } },
    { [&]{ return fs.startOver()->andPicture()->getFontCount(); },    [&]{ return fs.startOver()->notPicture()->getFontCount(); } },
    { [&]{ return fs.startOver()->andSingleChar()->getFontCount(); }, [&]{ return fs.startOver()->notSingleChar()->getFontCount(); } },
    { [&]{ return fs.startOver()->andMultiChar()->getFontCount(); },  [&]{ return fs.startOver()->notMultiChar()->getFontCount(); } },
    { [&]{ return fs.startOver()->andVariable()->getFontCount(); },   [&]{ return fs.startOver()->notVariable()->getFontCount(); } },
  };
  for(const P& p : pairs) {
    const int on = p.on(), off = p.off();
    QVERIFY(on  >= 0 && on  <= m_all);
    QVERIFY(off >= 0 && off <= m_all);
    QVERIFY2(on + off <= m_all, "and/not predicate pair overlapped");
  }
}

void TestFontSearch::getFonts_fontAt_qmlProtect()
{
  fs.startOver();
  QCOMPARE(fs.getFonts().size(), fs.getFontCount());
  if(fs.getFontCount() > 0)
    QCOMPARE(fs.fontAt(0), fs.getFonts().constFirst());

  QQmlEngine engine;
  fs.qmlProtect(&engine);
  QCOMPARE(QQmlEngine::objectOwnership(&fs), QQmlEngine::CppOwnership);
}

void TestFontSearch::qmlRegister_secondCallIsNoOp()
{
  // Private slot already run from the constructor; re-invoke to cover the latch.
  QVERIFY(QMetaObject::invokeMethod(&fs, "qmlRegister", Qt::DirectConnection));
}

QTEST_GUILESS_MAIN(TestFontSearch)
#include "tst_fontsearch.moc"
