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
#include <QCollator>
#include <QVariantList>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/pokemonstoragemodel.h>
#include <mvc/pokemonoverviewmodel.h>

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

  void pokemonOverview_columnsCountsTooltips();
};

void TestStorageModel::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();
}

void TestStorageModel::init()
{
  m_file = new FileManagement;
  loadInto(*m_file->data, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
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

// The "View All" overview: rows are alphabetized species, columns are the Party
// then each NON-EMPTY box, cells carry per-box counts, and each non-zero cell has
// a tooltip listing differing nicknames + an "...and ×N others" tail + a
// caught/traded split. Build a deterministic scene: empty every PC box, then fill
// box 0 with two mons of a species (S1) the party also holds -- one nicknamed +
// traded, one plain + caught.
void TestStorageModel::pokemonOverview_columnsCountsTooltips()
{
  auto* party   = m_file->data->dataExpanded->player->pokemon;
  auto* basics  = m_file->data->dataExpanded->player->basics;
  auto* storage = m_file->data->dataExpanded->storage;

  if(party->pokemon.size() < 1) QSKIP("party is empty");

  // Two distinct species from the party: S0 (party-only) and S1 (also placed in a box).
  const int S0 = party->pokemon.at(0)->species;
  int S1 = -1;
  for(PokemonBox* m : party->pokemon)
    if(m->species != S0) { S1 = m->species; break; }
  if(S1 < 0) QSKIP("party holds only one species");

  auto countInParty = [&](int sp) {
    int n = 0;
    for(PokemonBox* m : party->pokemon) if(m->species == sp) n++;
    return n;
  };
  const int partyS0 = countInParty(S0);
  const int partyS1 = countInParty(S1);

  // Empty every PC box, then fill box 0 with two S1 mons.
  for(int i = 0; i < storage->boxCount(); ++i)
    storage->boxAt(i)->reset();

  auto* box0 = storage->boxAt(0);
  box0->pokemonNew();
  box0->pokemonNew();
  PokemonBox* a = box0->pokemon.at(0);
  PokemonBox* b = box0->pokemon.at(1);

  // a: nicknamed + traded.   b: no nickname + caught (player's own OT).
  a->species = S1; a->nickname = QStringLiteral("Sparky"); a->nicknameChanged();
  a->otName = basics->playerName + QStringLiteral("X");  // force a different OT name
  a->otID = basics->playerID;

  b->species = S1; b->changeName(true);                  // sets nickname to the species default
  b->otName = basics->playerName;
  b->otID = basics->playerID;

  // Box 1 (index 1): two S0 mons, both caught (player's OT) + no nickname -- so the
  // S0/Box-2 cell must carry NO tooltip at all (nothing nicknamed, nothing traded).
  auto* box1 = storage->boxAt(1);
  box1->pokemonNew();
  box1->pokemonNew();
  for(PokemonBox* m : { box1->pokemon.at(0), box1->pokemon.at(1) }) {
    m->species = S0; m->changeName(true);
    m->otName = basics->playerName;
    m->otID = basics->playerID;
  }

  auto* ov = m_brg->pokemonOverviewModel;
  ov->rebuild();

  // ---- Columns: Party first, then only the non-empty boxes (box 0 + box 1). ----
  const QStringList cols = ov->columns();
  QCOMPARE(cols.size(), 3);
  QCOMPARE(cols.at(0), QStringLiteral("Party"));
  QCOMPARE(cols.at(1), QStringLiteral("Box 1"));
  QCOMPARE(cols.at(2), QStringLiteral("Box 2"));

  // ---- Locate the two species rows by display name. ----
  const QString nameS0 = party->pokemon.at(0)->speciesName();
  QString nameS1;
  for(PokemonBox* m : party->pokemon) if(m->species == S1) { nameS1 = m->speciesName(); break; }

  int rowS0 = -1, rowS1 = -1;
  for(int r = 0; r < ov->rowCount(QModelIndex()); ++r) {
    const QString nm = ov->data(ov->index(r), PokemonOverviewModel::NameRole).toString();
    if(nm == nameS0) rowS0 = r;
    if(nm == nameS1) rowS1 = r;
  }
  QVERIFY(rowS0 >= 0);
  QVERIFY(rowS1 >= 0);

  // ---- Counts: S0 is party-only (box cell 0 + empty tooltip); S1 spans both. ----
  const QVariantList s0Counts   = ov->data(ov->index(rowS0), PokemonOverviewModel::CountsRole).toList();
  const QVariantList s0Tips     = ov->data(ov->index(rowS0), PokemonOverviewModel::TooltipsRole).toList();
  QCOMPARE(s0Counts.at(0).toInt(), partyS0); // Party
  QCOMPARE(s0Counts.at(1).toInt(), 0);       // Box 1 (S1 only)
  QVERIFY(s0Tips.at(1).toString().isEmpty()); // a zero cell carries no tooltip
  QCOMPARE(s0Counts.at(2).toInt(), 2);        // Box 2: the two caught S0 mons
  QVERIFY2(s0Tips.at(2).toString().isEmpty(),  // all caught + no nickname -> NO tooltip
           qPrintable(s0Tips.at(2).toString()));

  const QVariantList s1Counts = ov->data(ov->index(rowS1), PokemonOverviewModel::CountsRole).toList();
  const QVariantList s1Tips   = ov->data(ov->index(rowS1), PokemonOverviewModel::TooltipsRole).toList();
  QCOMPARE(s1Counts.at(0).toInt(), partyS1); // Party
  QCOMPARE(s1Counts.at(1).toInt(), 2);       // Box 1: the two we added

  // ---- Tooltip on the Box-1 S1 cell: nickname list + others + caught/traded. ----
  const QString tip = s1Tips.at(1).toString();
  QVERIFY2(tip.contains(QStringLiteral("Sparky")), qPrintable(tip));
  QVERIFY2(tip.contains(QStringLiteral("×1 other")), qPrintable(tip)); // the un-nicknamed b
  QVERIFY2(tip.contains(QStringLiteral("×1 caught")), qPrintable(tip)); // b
  QVERIFY2(tip.contains(QStringLiteral("×1 traded")), qPrintable(tip)); // a

  // ---- Default sort is alphabetical (SortName); rows are in name order. ----
  QCollator coll; coll.setNumericMode(true); coll.setIgnorePunctuation(true);
  for(int r = 1; r < ov->rowCount(QModelIndex()); ++r) {
    const QString prev = ov->data(ov->index(r - 1), PokemonOverviewModel::NameRole).toString();
    const QString cur  = ov->data(ov->index(r),     PokemonOverviewModel::NameRole).toString();
    QVERIFY2(coll.compare(prev, cur) <= 0, "overview rows are not alphabetized");
  }

  // ---- The species-column sort cycles (same orders as the Pokedex screen) and
  // never adds/drops rows. ----
  const int nrows  = ov->rowCount(QModelIndex());
  const int before = ov->property("sortSelect").toInt();
  ov->sortCycle();
  QVERIFY(ov->property("sortSelect").toInt() != before);
  QVERIFY(!ov->sortLabel().isEmpty());
  QCOMPARE(ov->rowCount(QModelIndex()), nrows);
}

QTEST_GUILESS_MAIN(TestStorageModel)
#include "tst_storage_model.moc"
