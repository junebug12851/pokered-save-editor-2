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
 * @file tst_models.cpp
 * @brief First tests of the APP layer -- now possible because the bridge/mvc/engine
 *        code was extracted into the `appcore` static library. Covers two simple,
 *        DB-backed QAbstractListModels (types, natures): row counts, role names, and
 *        data lookups. Proves the app-layer is now unit-testable; more models follow.
 */

#include <QtTest>
#include <QModelIndex>

#include <pse-db/db.h>
#include <pse-db/pokemon.h>
#include <mvc/typesmodel.h>
#include <mvc/natureselectmodel.h>
#include <mvc/speciesselectmodel.h>
#include <mvc/moveselectmodel.h>
#include <mvc/statusselectmodel.h>
#include <mvc/pokemonstartersmodel.h>
#include <mvc/creditsmodel.h>
#include <mvc/itemselectmodel.h>
#include <mvc/mapselectmodel.h>

#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>

class TestModels : public QObject
{
  Q_OBJECT

private:
  SaveFile m_sf; // blank save -> supplies a live AreaMap for MapSelectModel

private slots:
  void initTestCase();
  void typesModel_hasRowsRolesAndData();
  void natureModel_has25NaturesAndRoundTrips();
  void speciesModel_listsAllSpeciesAndMaps();
  void moveModel_hasMovesAndMaps();
  void statusModel_hasRowsAndData();
  void startersModel_hasThreeStartersAndResolves();
  void creditsModel_loads();
  void itemSelectModel_listsItems();
  void mapSelectModel_listsMaps();
};

void TestModels::initTestCase()
{
  QVERIFY(DB::inst() != nullptr); // models read the game DBs
}

void TestModels::typesModel_hasRowsRolesAndData()
{
  TypesModel m;
  const int rows = m.rowCount(QModelIndex());
  QVERIFY2(rows > 0, "types model is empty");

  const QHash<int, QByteArray> roles = m.roleNames();
  QVERIFY(roles.contains(TypesModel::NameRole));
  QVERIFY(roles.contains(TypesModel::IndRole));

  const QVariant name = m.data(m.index(0, 0), TypesModel::NameRole);
  QVERIFY2(!name.toString().isEmpty(), "first type has empty name");
}

void TestModels::natureModel_has25NaturesAndRoundTrips()
{
  NatureSelectModel m;
  QCOMPARE(m.rowCount(QModelIndex()), 25); // Gen-1-derived natures

  const QVariant name = m.data(m.index(0, 0), NatureSelectModel::NameRole);
  QVERIFY(!name.toString().isEmpty());

  // natureToListIndex maps a nature value back to a valid row.
  const int row = m.natureToListIndex(0);
  QVERIFY(row >= 0 && row < 25);
}

void TestModels::speciesModel_listsAllSpeciesAndMaps()
{
  SpeciesSelectModel m;
  QVERIFY2(m.rowCount(QModelIndex()) > 100, "species picker too small");
  QVERIFY(!m.data(m.index(0, 0), SpeciesSelectModel::NameRole).toString().isEmpty());

  const int bulba = PokemonDB::inst()->getIndAt(QStringLiteral("Bulbasaur"))->ind;
  QVERIFY(m.speciesToListIndex(bulba) >= 0);
}

void TestModels::moveModel_hasMovesAndMaps()
{
  MoveSelectModel m;
  const int rows = m.rowCount(QModelIndex());
  QVERIFY2(rows > 0, "move picker is empty");
  // Row 0 may be a blank "None" entry; just require some row to have a real name.
  bool anyName = false;
  for(int i = 0; i < rows && !anyName; i++)
    if(!m.data(m.index(i, 0), MoveSelectModel::NameRole).toString().isEmpty())
      anyName = true;
  QVERIFY2(anyName, "no move row had a non-empty name");
  QVERIFY(m.moveToListIndex(1) >= 0);
}

void TestModels::statusModel_hasRowsAndData()
{
  StatusSelectModel m;
  QVERIFY2(m.rowCount(QModelIndex()) > 0, "status picker is empty");
  QVERIFY(!m.data(m.index(0, 0), StatusSelectModel::NameRole).toString().isEmpty());
  QVERIFY(m.statusToListIndex(0) >= 0);
}

void TestModels::startersModel_hasThreeStartersAndResolves()
{
  PokemonStartersModel m;
  // 4 rows: a leading "None" entry + the 3 canonical starters.
  QCOMPARE(m.rowCount(QModelIndex()), 4);
  QCOMPARE(m.data(m.index(0, 0), PokemonStartersModel::NameRole).toString(), QStringLiteral("None"));
  for(int row = 1; row < 4; row++) // rows 1..3 are the real starters
    QVERIFY(!m.data(m.index(row, 0), PokemonStartersModel::NameRole).toString().isEmpty());
  // getMon indexes the 3-element starters[] (row-1); 0..2 are valid.
  for(int i = 0; i < 3; i++)
    QVERIFY2(m.getMon(i) != nullptr, qPrintable(QStringLiteral("starter %1 did not resolve").arg(i)));
  QCOMPARE(m.valToIndex(0), 0); // value 0 maps to the "None" row
}

void TestModels::creditsModel_loads()
{
  CreditsModel m;
  const int rows = m.rowCount(QModelIndex());
  QVERIFY2(rows >= 0, "credits model returned a negative row count");
  if(rows > 0)
    QVERIFY(m.data(m.index(0, 0), CreditsModel::NameRole).isValid());
}

void TestModels::itemSelectModel_listsItems()
{
  ItemSelectModel m;
  const int rows = m.rowCount(QModelIndex());
  QVERIFY2(rows > 0, "item picker is empty");
  bool anyName = false;
  for(int i = 0; i < rows && !anyName; i++)
    if(!m.data(m.index(i, 0), ItemSelectModel::NameRole).toString().isEmpty())
      anyName = true;
  QVERIFY2(anyName, "no item row had a non-empty name");
  QVERIFY(m.itemToListIndex(1) >= 0);
}

void TestModels::mapSelectModel_listsMaps()
{
  // Construct against the blank save's live AreaMap (used for current-map highlight).
  MapSelectModel m(m_sf.dataExpanded->area->map);
  const int rows = m.rowCount(QModelIndex());
  QVERIFY2(rows > 0, "map picker is empty");
  bool anyName = false;
  for(int i = 0; i < rows && !anyName; i++)
    if(!m.data(m.index(i, 0), MapSelectModel::NameRole).toString().isEmpty())
      anyName = true;
  QVERIFY2(anyName, "no map row had a non-empty name");
  QVERIFY(m.mapToListIndex(0) >= 0);
}

QTEST_GUILESS_MAIN(TestModels)
#include "tst_models.moc"
