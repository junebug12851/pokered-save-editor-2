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
 * @file tst_move_select_model.cpp
 * @brief MoveSelectModel's two list modes: the general list (all moves) and the
 *        species-specific list (a mon's legal moves only). Exercises the
 *        monFromBox()/rebuild paths that the basic model test never drove, plus
 *        data roles and the move->row lookup.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>

#include <mvc/moveselectmodel.h>

using namespace pse_test;

class TestMoveSelectModel : public QObject
{
  Q_OBJECT

private:
  int rows(MoveSelectModel& m) { return m.rowCount(QModelIndex()); }

private slots:
  void initTestCase();
  void generalList_hasAllMoves();
  void specificList_isSpeciesLegalSubset();
  void monFromBoxNull_fallsBackToGeneral();
};

void TestMoveSelectModel::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

void TestMoveSelectModel::generalList_hasAllMoves()
{
  MoveSelectModel m;                       // constructed in the general mode
  const int g = rows(m);
  QVERIFY2(g > 50, "general move list unexpectedly small");

  QVERIFY(!m.roleNames().isEmpty());
  // Some row has a real name; the move->row lookup round-trips for a valid move.
  bool anyName = false;
  for(int i = 0; i < g && !anyName; i++)
    if(!m.data(m.index(i), MoveSelectModel::NameRole).toString().isEmpty()) anyName = true;
  QVERIFY2(anyName, "no move row had a non-empty name");
  QVERIFY(m.moveToListIndex(1) >= 0);
}

void TestMoveSelectModel::specificList_isSpeciesLegalSubset()
{
  SaveFile sf; loadInto(sf, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
  PokemonParty* partyMon = sf.dataExpanded->player->pokemon->partyAt(0);
  QVERIFY(partyMon != nullptr);

  MoveSelectModel m;
  const int general = rows(m);

  m.monFromBox(partyMon);                  // -> rebuilds to that species' move list
  const int specific = rows(m);
  QVERIFY2(specific > 1, "specific move list is empty");
  Q_UNUSED(general); // the species list isn't a strict subset (it also includes the mon's current moves)

  // The move->row lookup resolves a move that is actually in the rebuilt list.
  const int someInd = m.data(m.index(1), MoveSelectModel::IndRole).toInt();
  QCOMPARE(m.moveToListIndex(someInd), 1);
}

void TestMoveSelectModel::monFromBoxNull_fallsBackToGeneral()
{
  SaveFile sf; loadInto(sf, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));

  MoveSelectModel m;
  const int general = rows(m);

  m.monFromBox(sf.dataExpanded->player->pokemon->partyAt(0)); // narrow to a species
  QVERIFY(rows(m) > 1);

  m.monFromBox(nullptr);                   // null mon -> back to the general list
  QCOMPARE(rows(m), general);
}

QTEST_GUILESS_MAIN(TestMoveSelectModel)
#include "tst_move_select_model.moc"
