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
#include <mvc/typesmodel.h>
#include <mvc/natureselectmodel.h>

class TestModels : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void typesModel_hasRowsRolesAndData();
  void natureModel_has25NaturesAndRoundTrips();
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

QTEST_GUILESS_MAIN(TestModels)
#include "tst_models.moc"
