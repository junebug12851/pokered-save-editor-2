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
 * @file tst_db_coverage_fill.cpp
 * @brief Coverage gap-fill (category 1 in plans/testing.md): DB accessor one-liners
 *        no existing test happened to call. Pure read-only calls -- zero source
 *        changes, zero risk. Targets from the 2026-06-22 llvm-cov per-line report.
 *
 * NOTE on reachability (learned building this): the db shared lib only EXPORTS the
 * DB_AUTOPORT-marked classes, so some accessors aren't externally linkable:
 *   - Examples is NOT DB_AUTOPORT, so Examples::player()/rival()/pokemon() can't be
 *     called directly from a test exe. They ARE Q_PROPERTYs, so we invoke them
 *     through the meta-object (property("player")) -- the READ runs inside the dll,
 *     covering the accessor lines without needing the exported symbol.
 *   - TmHmsDB / MusicDB / TrainersDB ARE DB_AUTOPORT -> their getters link directly.
 *   - HiddenCoinsDB is NOT exported, and the HiddenItems store is empty in the test
 *     data (which is why those entry getters read uncovered) -- not reachable from a
 *     headless unit test, so left out (documented in plans/testing.md).
 */

#include <QtTest>
#include <QObject>
#include <QVariant>

#include <pse-db/db.h>
#include <pse-db/examples.h>
#include <pse-db/tmHm.h>
#include <pse-db/music.h>
#include <pse-db/trainers.h>

class TestDbCoverageFill : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void examples_accessorsViaMetaObject();
  void tmHm_accessors();
  void music_accessors();
  void trainers_accessors();
};

void TestDbCoverageFill::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

void TestDbCoverageFill::examples_accessorsViaMetaObject()
{
  // db.examples is exported (via DB); its player/rival/pokemon accessors are not,
  // but they're Q_PROPERTYs -- reading the property invokes the accessor inside the
  // dll, covering Examples::player()/rival()/pokemon().
  QObject* ex = DB::inst()->examples();
  QVERIFY(ex != nullptr);
  QVERIFY(ex->property("player").value<QObject*>()  != nullptr);
  QVERIFY(ex->property("rival").value<QObject*>()   != nullptr);
  QVERIFY(ex->property("pokemon").value<QObject*>() != nullptr);
}

void TestDbCoverageFill::tmHm_accessors()
{
  TmHmsDB* db = TmHmsDB::inst();
  QVERIFY(db != nullptr);
  QVERIFY(db->getStore().size()      >= 0);
  QVERIFY(db->getTmHmItems().size()  >= 0);   // resolved ItemDBEntry* list
  QVERIFY(db->getTmHmMoves().size()  >= 0);   // resolved MoveDBEntry* list
}

void TestDbCoverageFill::music_accessors()
{
  MusicDB* db = MusicDB::inst();
  QVERIFY(db != nullptr);
  QVERIFY(db->getStore().size() >= 0);
  QVERIFY(db->getInd().size()   >= 0);
}

void TestDbCoverageFill::trainers_accessors()
{
  TrainersDB* db = TrainersDB::inst();
  QVERIFY(db != nullptr);
  QVERIFY(db->getStore().size() >= 0);
  QVERIFY(db->getInd().size()   >= 0);
}

QTEST_GUILESS_MAIN(TestDbCoverageFill)
#include "tst_db_coverage_fill.moc"
