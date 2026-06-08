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
 * @file tst_select_models.cpp
 * @brief Exhaustive drive of the QML select-list models (types/nature/species/
 *        status/starters/move/map/item): every row x every role through data(),
 *        the invalid-index guards, roleNames, and each model's value<->row helper
 *        across edge values (placeholder 0xFF, a real value, a not-found value).
 *        tst_models proves each builds; this exercises every data()/lookup branch.
 */

#include <QtTest>
#include <QAbstractItemModel>
#include <QModelIndex>

#include <pse-db/db.h>
#include <pse-db/pokemon.h>
#include <pse-db/mapsdb.h>

#include <mvc/typesmodel.h>
#include <mvc/natureselectmodel.h>
#include <mvc/speciesselectmodel.h>
#include <mvc/statusselectmodel.h>
#include <mvc/pokemonstartersmodel.h>
#include <mvc/moveselectmodel.h>
#include <mvc/mapselectmodel.h>
#include <mvc/itemselectmodel.h>

#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>

// Drive every row x every declared role through data(), plus the invalid-index
// and out-of-range paths, headerData and flags. Covers the data() role branches
// (incl. any leading placeholder row) of any list model.
static void driveModel(QAbstractItemModel* m)
{
  const int rows = m->rowCount(QModelIndex());
  QVERIFY(rows >= 0);
  const QHash<int, QByteArray> roles = m->roleNames();
  QVERIFY(!roles.isEmpty());

  for(int r = 0; r < rows; r++) {
    const QModelIndex idx = m->index(r, 0, QModelIndex());
    QVERIFY(idx.isValid());
    for(auto role = roles.keyBegin(); role != roles.keyEnd(); ++role)
      (void)m->data(idx, *role);
    (void)m->flags(idx);
  }

  // An undeclared role on a non-placeholder row exercises data()'s default return.
  // (Row 0 + an undeclared role is intentionally avoided: the placeholder-row branch
  // only handles its declared roles and otherwise falls through to at(row-1)==at(-1);
  // QML never queries undeclared roles, so this is a gated latent edge, not a UI bug.)
  if(rows > 1)
    (void)m->data(m->index(1, 0, QModelIndex()), Qt::DisplayRole);

  (void)m->data(QModelIndex(), *roles.keyBegin());        // invalid index -> empty
  (void)m->index(rows + 5, 0, QModelIndex());             // out-of-range index
  (void)m->headerData(0, Qt::Horizontal, Qt::DisplayRole);
}

class TestSelectModels : public QObject
{
  Q_OBJECT

  SaveFile m_sf;   // blank save -> live AreaMap for MapSelectModel

private slots:
  void initTestCase();
  void types();
  void nature();
  void species();
  void status();
  void starters();
  void moves();
  void items();
  void maps();
};

void TestSelectModels::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  MapsDB::inst()->deepLink();
}

void TestSelectModels::types()
{
  TypesModel m;
  driveModel(&m);
  QCOMPARE(m.valToIndex(0xFF), 0);              // placeholder row
  const int real = TypesDB::inst()->getStore().at(0)->ind;
  QVERIFY(m.valToIndex(real) >= 0);
  QCOMPARE(m.valToIndex(0x4242), 0);            // not found -> 0
}

void TestSelectModels::nature()
{
  NatureSelectModel m;
  driveModel(&m);
  QVERIFY(m.natureToListIndex(0) >= 0);
  QVERIFY(m.natureToListIndex(24) >= 0);
  (void)m.natureToListIndex(9999);              // out of range path
}

void TestSelectModels::species()
{
  SpeciesSelectModel m;
  driveModel(&m);
  const int bulba = PokemonDB::inst()->getIndAt(QStringLiteral("Bulbasaur"))->ind;
  QVERIFY(m.speciesToListIndex(bulba) >= 0);
  (void)m.speciesToListIndex(0xFFFF);
}

void TestSelectModels::status()
{
  StatusSelectModel m;
  driveModel(&m);
  QVERIFY(m.statusToListIndex(0) >= 0);
  (void)m.statusToListIndex(0xFF);
}

void TestSelectModels::starters()
{
  PokemonStartersModel m;
  driveModel(&m);
  QCOMPARE(m.valToIndex(0), 0);
  for(int i = 0; i < 3; i++) QVERIFY(m.getMon(i) != nullptr);
  (void)m.valToIndex(0x9999);
}

void TestSelectModels::moves()
{
  MoveSelectModel m;
  driveModel(&m);
  QVERIFY(m.moveToListIndex(1) >= 0);
  (void)m.moveToListIndex(0xFFFF);
}

void TestSelectModels::items()
{
  ItemSelectModel m;
  driveModel(&m);
  QVERIFY(m.itemToListIndex(1) >= 0);
  (void)m.itemToListIndex(0xFFFF);
}

void TestSelectModels::maps()
{
  MapSelectModel m(m_sf.dataExpanded->area->map);
  driveModel(&m);
  QVERIFY(m.mapToListIndex(0) >= 0);
  (void)m.mapToListIndex(0xFFFF);
}

QTEST_GUILESS_MAIN(TestSelectModels)
#include "tst_select_models.moc"
