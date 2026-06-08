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
 * @file tst_storage.cpp
 * @brief Coverage for Storage (the PC) beyond what load/save round-trips reach:
 *        the flattened 0..11 box space (boxCount/boxAt across both 6-box sets),
 *        freeSpace()'s room invariant, depositPokemon() success and the
 *        all-full -> false path, and the randomize verbs (randomize /
 *        randomizeItems / randomizePokemon) running without crashing.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

using namespace pse_test;

class TestStorage : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();
  void boxCount_boxAt_spanBothSets();
  void freeSpace_roomInvariant();
  void deposit_succeedsIntoFreeBox();
  void deposit_failsWhenTargetFull();
  void randomizeVerbs_noCrash();
};

void TestStorage::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestStorage::boxCount_boxAt_spanBothSets()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* st = sf.dataExpanded->storage;

  QCOMPARE(st->boxCount(), int(maxPokemonBoxes)); // 12

  for(int i = 0; i < st->boxCount(); i++)
    QVERIFY2(st->boxAt(i) != nullptr, qPrintable(QStringLiteral("box %1 null").arg(i)));

  // The flattened space crosses the two 6-box sets: low indices are set A,
  // 6..11 are set B, so a same-offset pair across the boundary is distinct.
  QVERIFY(st->boxAt(0) != st->boxAt(6));
  QVERIFY(st->boxAt(5) != st->boxAt(11));
}

void TestStorage::freeSpace_roomInvariant()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* st = sf.dataExpanded->storage;

  PokemonStorageBox* fs = st->freeSpace();
  if(fs != nullptr)
    QVERIFY(fs->pokemonCount() < fs->pokemonMax()); // a "free" box really has room
}

void TestStorage::deposit_succeedsIntoFreeBox()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* st = sf.dataExpanded->storage;

  // Drive the simple (unformatted) freeSpace path: it returns the current box if
  // that box has room. Point curBox at a box that has room.
  st->boxesFormatted = false;
  int target = -1;
  for(int i = 0; i < st->boxCount(); i++) {
    if(st->boxAt(i)->pokemonCount() < st->boxAt(i)->pokemonMax()) { target = i; break; }
  }
  QVERIFY2(target >= 0, "no PC box has room in the fixture");
  st->curBox = target;

  PokemonStorageBox* box = st->boxAt(target);
  const int before = box->pokemonCount();

  QVERIFY(st->depositPokemon(new PokemonBox())); // owned by the box now
  QCOMPARE(box->pokemonCount(), before + 1);
}

void TestStorage::deposit_failsWhenTargetFull()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* st = sf.dataExpanded->storage;

  st->boxesFormatted = false;
  st->curBox = 0;
  PokemonStorageBox* box = st->boxAt(0);

  // Fill the current box to capacity (direct append; owned by the box).
  while(box->pokemonCount() < box->pokemonMax())
    box->pokemon.append(new PokemonBox());

  QVERIFY(st->freeSpace() == nullptr);          // no room anywhere reachable
  PokemonBox* orphan = new PokemonBox();
  QVERIFY(!st->depositPokemon(orphan));          // rejected
  delete orphan;                                 // not adopted -> free it
}

void TestStorage::randomizeVerbs_noCrash()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* st = sf.dataExpanded->storage;
  auto* basics = sf.dataExpanded->player->basics;

  st->randomize(basics);          // reset + randomizeItems + randomizePokemon
  QCOMPARE(st->boxCount(), int(maxPokemonBoxes));
  for(int i = 0; i < st->boxCount(); i++)
    QVERIFY(st->boxAt(i) != nullptr);

  st->randomizeItems();           // exercise the sub-verbs directly too
  st->randomizePokemon(basics);
  QVERIFY(true);                  // reaching here without a crash is the assertion
}

QTEST_GUILESS_MAIN(TestStorage)
#include "tst_storage.moc"
