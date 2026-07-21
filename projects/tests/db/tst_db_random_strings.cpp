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
 * @file tst_db_random_strings.cpp
 * @brief The AbstractRandomString sources (random player/Pokemon names + example
 *        player/Pokemon/rival presets) the randomizer pulls from: store accessors
 *        incl. the negative-index guard (fixed here), and randomExample() returning
 *        a string from the list while drawing WITHOUT replacement (it removes the
 *        picked entry, so the store shrinks by one).
 */

#include <QtTest>
#include <QVector>
#include <QString>

#include <pse-db/db.h>
#include <pse-db/entries/abstractrandomstring.h>
#include <pse-db/entries/namesplayer.h>
#include <pse-db/entries/namespokemon.h>
#include <pse-db/entries/examplesplayer.h>
#include <pse-db/entries/examplespokemon.h>
#include <pse-db/entries/examplesrival.h>

#define CHECK_RND(CLASS) do {                                                      \
    AbstractRandomString* s = CLASS::inst();                                       \
    QVERIFY2(s != nullptr, #CLASS "::inst() null");                                \
    const int n = s->getStoreSize();                                               \
    QVERIFY2(n > 0, #CLASS " has no strings");                                      \
    QCOMPARE(s->getStore().size(), n);                                             \
    QVERIFY2(!s->getStoreAt(0).isEmpty(), #CLASS " first string empty");           \
    QVERIFY2(s->getStoreAt(-1).isEmpty(), #CLASS " negative index not empty");     \
    QVERIFY2(s->getStoreAt(n + 50).isEmpty(), #CLASS " oob index not empty");      \
    const QVector<QString> before = s->getStore();                                 \
    const QString picked = s->randomExample();                                     \
    QVERIFY2(!picked.isEmpty(), #CLASS " randomExample empty");                     \
    QVERIFY2(before.contains(picked), #CLASS " randomExample not from the store");  \
    QCOMPARE(s->getStoreSize(), n - 1); /* drawn without replacement */            \
  } while(0)

class TestDbRandomStrings : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase() { QVERIFY(DB::inst() != nullptr); }
  void allRandomStringSources_storeAndRandomExample();
};

void TestDbRandomStrings::allRandomStringSources_storeAndRandomExample()
{
  CHECK_RND(NamesPlayer);
  CHECK_RND(NamesPokemon);
  CHECK_RND(ExamplesPlayer);
  CHECK_RND(ExamplesPokemon);
  CHECK_RND(ExamplesRival);
}

QTEST_GUILESS_MAIN(TestDbRandomStrings)
#include "tst_db_random_strings.moc"
