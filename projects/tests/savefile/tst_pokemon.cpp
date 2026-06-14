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
 * @file tst_pokemon.cpp
 * @brief Phase-2 coverage of the Pokemon record -- the deepest, most split-apart
 *        region of the save (BOX_MON / PARTY_MON), and historically where a
 *        write-corruption bug lived (cb6fc99 "party data mangled on write").
 *
 * Value round-trips of every stored field of a Pokemon: species, level, status,
 * hp, catch rate, OT id/name, exp, the five stat-exp ("EV") values, the four DVs,
 * all four move slots (id/pp/ppUp), nickname, and -- for party mons -- the five
 * pre-generated battle stats. set -> flatten -> re-expand -> assert the value
 * survived the raw encoding and the split record/name-table layout.
 *
 * Whole-save byte isolation for a single Pokemon edit is covered transitively by
 * tst_roundtrip's identity test (flatten is byte-perfect); here we focus on the
 * many fields' value fidelity.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>

using namespace pse_test;

class TestPokemon : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

  // Set every shared BOX_MON field on a mon to a known, distinct value.
  static void setBoxFields(PokemonBox* p)
  {
    p->species   = 151;     // Mew
    p->level     = 50;
    p->status    = 8;       // a status bit, stored as a byte
    p->hp        = 123;
    p->catchRate = 45;
    p->otID      = 4242;
    p->exp       = 125000u; // 24-bit
    p->hpExp     = 11111;   // 16-bit stat-exp ("EVs")
    p->atkExp    = 22222;
    p->defExp    = 33333;
    p->spdExp    = 44444;
    p->spExp     = 55555;
    p->dvSet(0, 15);
    p->dvSet(1, 0);
    p->dvSet(2, 7);
    p->dvSet(3, 10);
    const int mid[4] = {1, 2, 3, 4};
    // PP must stay within each move's max (engine clamps PP to the move's cap,
    // faithfully to the game). moves 1-4 all have base PP >= 10, so <=9 is safe.
    const int mpp[4] = {3, 5, 7, 9};
    const int mup[4] = {0, 1, 2, 3};    // ppUp is 2 bits (0..3)
    for(int k = 0; k < maxMoves; k++) {
      PokemonMove* m = p->movesAt(k);
      m->moveID = mid[k];
      m->pp     = mpp[k];
      m->ppUp   = mup[k];
    }
    p->otName   = QStringLiteral("OAK");
    p->nickname = QStringLiteral("ZAP");
  }

  // Verify the shared BOX_MON fields survived a round-trip.
  static void checkBoxFields(PokemonBox* p)
  {
    QCOMPARE(p->species, 151);
    QCOMPARE(p->level, 50);
    QCOMPARE(p->status, 8);
    QCOMPARE(p->hp, 123);
    QCOMPARE(p->catchRate, 45);
    QCOMPARE(p->otID, 4242);
    QCOMPARE(p->exp, 125000u);
    QCOMPARE(p->hpExp, 11111);
    QCOMPARE(p->atkExp, 22222);
    QCOMPARE(p->defExp, 33333);
    QCOMPARE(p->spdExp, 44444);
    QCOMPARE(p->spExp, 55555);
    QCOMPARE(p->dvAt(0), 15);
    QCOMPARE(p->dvAt(1), 0);
    QCOMPARE(p->dvAt(2), 7);
    QCOMPARE(p->dvAt(3), 10);
    const int mid[4] = {1, 2, 3, 4};
    const int mpp[4] = {3, 5, 7, 9};
    const int mup[4] = {0, 1, 2, 3};
    for(int k = 0; k < maxMoves; k++) {
      PokemonMove* m = p->movesAt(k);
      QCOMPARE(m->moveID, mid[k]);
      QCOMPARE(m->pp, mpp[k]);
      QCOMPARE(m->ppUp, mup[k]);
    }
    QCOMPARE(p->otName, QStringLiteral("OAK"));
    QCOMPARE(p->nickname, QStringLiteral("ZAP"));
  }

private slots:
  void initTestCase();

  void partyMon_roundTrip();
  void boxMon_roundTrip();
};

void TestPokemon::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestPokemon::partyMon_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  PlayerPokemon* party = sf.dataExpanded->player->pokemon;

  if(party->pokemonCount() == 0)
    QSKIP("fixture has an empty party");

  PokemonParty* p = party->partyAt(0);
  QVERIFY(p != nullptr);

  setBoxFields(p);
  // Party-only stored battle stats (boxes recompute these; party records store them).
  p->maxHP   = 222;
  p->attack  = 111;
  p->defense = 99;
  p->speed   = 120;
  p->special = 133;

  sf.flattenData();
  sf.expandData();

  PokemonParty* p2 = sf.dataExpanded->player->pokemon->partyAt(0);
  QVERIFY(p2 != nullptr);
  checkBoxFields(p2);
  QCOMPARE(p2->maxHP, 222);
  QCOMPARE(p2->attack, 111);
  QCOMPARE(p2->defense, 99);
  QCOMPARE(p2->speed, 120);
  QCOMPARE(p2->special, 133);
}

void TestPokemon::boxMon_roundTrip()
{
  SaveFile sf; loadInto(sf, m_orig);
  Storage* storage = sf.dataExpanded->storage;

  // Find a non-empty PC box, preferring one that isn't the active box (the active
  // box is also mirrored in bank 1, which we don't want to entangle this test with).
  int boxIdx = -1;
  for(int i = 0; i < storage->boxCount(); i++) {
    PokemonStorageBox* b = storage->boxAt(i);
    if(b != nullptr && b->pokemonCount() > 0 && i != storage->curBox) { boxIdx = i; break; }
  }
  if(boxIdx < 0)
    for(int i = 0; i < storage->boxCount(); i++) {
      PokemonStorageBox* b = storage->boxAt(i);
      if(b != nullptr && b->pokemonCount() > 0) { boxIdx = i; break; }
    }
  if(boxIdx < 0)
    QSKIP("fixture has no boxed Pokemon");

  PokemonBox* m = storage->boxAt(boxIdx)->pokemonAt(0);
  QVERIFY(m != nullptr);
  setBoxFields(m);

  sf.flattenData();
  sf.expandData();

  PokemonBox* m2 = sf.dataExpanded->storage->boxAt(boxIdx)->pokemonAt(0);
  QVERIFY(m2 != nullptr);
  checkBoxFields(m2);
}

QTEST_GUILESS_MAIN(TestPokemon)
#include "tst_pokemon.moc"
