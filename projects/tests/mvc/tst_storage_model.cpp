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
 * @file tst_storage_model.cpp
 * @brief The PokemonStorageModel bulk "checked" operations -- the PC/party editing
 *        verbs that QML drives via per-row checkboxes: toggle-all, move-to-top/
 *        bottom, delete, and transfer to the paired box. Built on a real Bridge over
 *        the BaseSAV party (so there are mons to operate on); ordering is verified by
 *        object-pointer identity, robust to duplicate species. A fresh fixture per
 *        test keeps each mutation isolated.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/pokemonstoragemodel.h>

using namespace pse_test;

class TestStorageModel : public QObject
{
  Q_OBJECT

private:
  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;
  PokemonStorageModel* m_sm = nullptr; // model1, shown on the party

  void check(int row, bool on)
  {
    m_sm->setData(m_sm->index(row), QVariant(on), PokemonStorageModel::CheckedRole);
  }

private slots:
  void initTestCase();
  void init();      // fresh save+bridge per test
  void cleanup();

  void toggleAll_flipsHasChecked();
  void moveToBottom_movesCheckedDown();
  void moveToTop_movesCheckedUp();
  void delete_removesCheckedMon();
  void transfer_movesToPairedBox();
};

void TestStorageModel::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();
}

void TestStorageModel::init()
{
  m_file = new FileManagement;
  loadInto(*m_file->data, readSaveBytes(QStringLiteral("BaseSAV.sav")));
  m_brg = new Bridge(m_file);
  m_sm = m_brg->pokemonStorageModel1;
  m_sm->switchBox(PokemonStorageModel::PartyBox); // the party always has mons in BaseSAV
}

void TestStorageModel::cleanup()
{
  delete m_brg; m_brg = nullptr;
  delete m_file; m_file = nullptr;
  m_sm = nullptr;
}

void TestStorageModel::toggleAll_flipsHasChecked()
{
  QVERIFY(!m_sm->hasChecked());
  m_sm->checkedToggleAll();
  QVERIFY(m_sm->hasChecked());
  m_sm->checkedToggleAll();
  QVERIFY(!m_sm->hasChecked());
}

void TestStorageModel::moveToBottom_movesCheckedDown()
{
  auto* box = m_sm->getCurBox();
  const int n = box->pokemon.size();
  if(n < 2) QSKIP("party has fewer than 2 mons");

  PokemonBox* first = box->pokemon.at(0);
  check(0, true);
  m_sm->checkedMoveToBottom();

  QCOMPARE(box->pokemon.size(), n);                 // count preserved
  QCOMPARE(box->pokemon.at(n - 1), first);          // the checked mon is now last
}

void TestStorageModel::moveToTop_movesCheckedUp()
{
  auto* box = m_sm->getCurBox();
  const int n = box->pokemon.size();
  if(n < 2) QSKIP("party has fewer than 2 mons");

  PokemonBox* last = box->pokemon.at(n - 1);
  check(n - 1, true);
  m_sm->checkedMoveToTop();

  QCOMPARE(box->pokemon.size(), n);
  QCOMPARE(box->pokemon.at(0), last);               // the checked mon is now first
}

void TestStorageModel::delete_removesCheckedMon()
{
  auto* box = m_sm->getCurBox();
  const int n = box->pokemon.size();
  if(n < 2) QSKIP("party has fewer than 2 mons");

  PokemonBox* doomed = box->pokemon.at(n - 1);
  check(n - 1, true);
  m_sm->checkedDelete();

  QCOMPARE(box->pokemon.size(), n - 1);
  QVERIFY2(!box->pokemon.contains(doomed), "deleted mon still present in the box");
}

void TestStorageModel::transfer_movesToPairedBox()
{
  auto* src = m_sm->getCurBox();                    // party
  auto* dst = m_brg->pokemonStorageModel2->getCurBox(); // a PC box (switched to box 0 by the Bridge)
  if(src == dst) QSKIP("both models share a box");
  if(src->pokemon.size() < 2) QSKIP("party has fewer than 2 mons");
  if(dst->isFull()) QSKIP("destination box is full");

  const int srcBefore = src->pokemon.size();
  const int dstBefore = dst->pokemon.size();
  const int movedSpecies = src->pokemon.at(0)->species;

  check(0, true);
  m_sm->checkedTransfer();

  // Transferring party -> PC re-homes the mon into a new PokemonBox object (party
  // mons are PokemonParty), so identity isn't preserved -- verify by count and that
  // a mon of the moved species now exists in the destination.
  QCOMPARE(src->pokemon.size(), srcBefore - 1);
  QCOMPARE(dst->pokemon.size(), dstBefore + 1);
  bool found = false;
  for(PokemonBox* m : dst->pokemon)
    if(m->species == movedSpecies) { found = true; break; }
  QVERIFY2(found, "no mon of the transferred species found in destination box");
}

QTEST_GUILESS_MAIN(TestStorageModel)
#include "tst_storage_model.moc"
