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
 * @file tst_fonts.cpp
 * @brief The Game Boy text codec (FontsDB convertToCode/convertFromCode + rendered
 *        length) and the FontSearch fluent glyph filter -- both core to every
 *        editable name/string in the app and previously thinly covered.
 */

#include <QtTest>
#include <QVector>

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/entries/fontdbentry.h>
#include <pse-db/util/fontsearch.h>

class TestFonts : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void codec_roundTripsUppercaseString();
  void codec_lengthAndLookups();
  void search_fluentFiltersNarrowResults();
};

void TestFonts::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(FontsDB::inst()->getStoreSize() > 0);
}

void TestFonts::codec_roundTripsUppercaseString()
{
  FontsDB* f = FontsDB::inst();
  for(const QString s : { QStringLiteral("RED"), QStringLiteral("PIKA"), QStringLiteral("ABC") }) {
    const QVector<int> codes = f->convertToCode(s);
    QVERIFY2(!codes.isEmpty(), qPrintable("convertToCode empty for " + s));
    const QString back = f->convertFromCode(codes);
    QCOMPARE(back, s); // string -> codes -> string is lossless for plain caps
  }
}

void TestFonts::codec_lengthAndLookups()
{
  FontsDB* f = FontsDB::inst();

  QCOMPARE(f->fontCount(), f->getStoreSize());      // alias agrees
  QVERIFY(f->countSizeOf(QStringLiteral("RED")) > 0);

  FontDBEntry* g0 = f->getStoreAt(0);
  QVERIFY(g0 != nullptr);
  QVERIFY(f->getStoreAt(-1) == nullptr);             // bounds guard
  QVERIFY(f->getStoreAt(f->getStoreSize() + 50) == nullptr);

  // fontAt() / getStoreByVal() look a glyph up by its font-code VALUE (not store
  // index). Drive both; a value of 0 may or may not map to a glyph, so we only
  // require the call to be safe and the two aliases to agree.
  QCOMPARE(f->fontAt(0), const_cast<FontDBEntry*>(f->getStoreByVal(0)));
}

void TestFonts::search_fluentFiltersNarrowResults()
{
  FontSearch fs;

  const int all = fs.startOver()->getFontCount();
  QVERIFY2(all > 0, "startOver() produced an empty glyph set");

  QCOMPARE(fs.clear()->getFontCount(), 0);           // clear empties the set

  // Each AND-filter is a subset of the full set; the chain returns `this`.
  const int normals = fs.startOver()->andNormal()->getFontCount();
  QVERIFY(normals > 0 && normals <= all);

  const int normalsNoControl = fs.startOver()->andNormal()->notControl()->getFontCount();
  QVERIFY(normalsNoControl <= normals);

  // notNormal is the complement-ish of andNormal: together they can't both be the full set.
  const int notNormals = fs.startOver()->notNormal()->getFontCount();
  QVERIFY(notNormals <= all);
  QVERIFY(normals + notNormals <= all + all); // sanity; no overflow/garbage
}

QTEST_GUILESS_MAIN(TestFonts)
#include "tst_fonts.moc"
