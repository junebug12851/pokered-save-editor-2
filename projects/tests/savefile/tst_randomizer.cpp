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
 * @file tst_randomizer.cpp
 * @brief Phase-7 randomizer invariants.
 *
 * The flagship randomizer (`SaveFile::randomizeExpansion()`) is a work in progress
 * and currently crashes on the MAP paths (Area/AreaSprites/AreaWarps -> SpriteData/
 * MapSearch). Two map bugs were already found via tests: `MapSearch::isType()`
 * null-deref (fixed) and `SpriteData::load()` null-deref on an unresolved
 * `getToSprite()`/`getToTrainer()`/`getToPokemon()` link (OPEN, status.md). The full
 * end-to-end randomize + its "must not touch a save byte" check live in tst_verbs,
 * QSKIP'd until that map crash-chain is finished.
 *
 * Here we test the randomizer COMPONENTS that already work, with real invariants:
 * trainer-basics randomization, the pokedex, and a freshly-rolled starter Pokemon.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/pokemon.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/player/playerpokedex.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

using namespace pse_test;

class TestRandomizer : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;
  int m_bulba = 0, m_charm = 0, m_squir = 0;

  bool isStarter(int species) const
  {
    return species == m_bulba || species == m_charm || species == m_squir;
  }

private slots:
  void initTestCase();
  void basicsRandomize_holdsInvariants();
  void pokedexRandomize_countsStaySane();
  void newStarterPokemon_isAValidStarter();
  void fullRandomizeExpansion_runsAndHoldsInvariants();
};

void TestRandomizer::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);

  m_bulba = PokemonDB::inst()->getIndAt(QStringLiteral("Bulbasaur"))->ind;
  m_charm = PokemonDB::inst()->getIndAt(QStringLiteral("Charmander"))->ind;
  m_squir = PokemonDB::inst()->getIndAt(QStringLiteral("Squirtle"))->ind;
}

void TestRandomizer::basicsRandomize_holdsInvariants()
{
  SaveFile sf; loadInto(sf, m_orig);
  PlayerBasics* b = sf.dataExpanded->player->basics;

  // Run many times: the constrained randomizer's promises must hold every time.
  for(int iter = 0; iter < 50; iter++) {
    b->randomize();

    QVERIFY2(b->money >= 100 && b->money <= 6000,
             qPrintable(QStringLiteral("money out of range: %1").arg(b->money)));
    QVERIFY2(b->coins >= 0 && b->coins <= 100,
             qPrintable(QStringLiteral("coins out of range: %1").arg(b->coins)));
    QVERIFY2(!b->getPlayerName().isEmpty(), "random name is empty");

    // Badges are earned linearly in gym order: always at least the first badge
    // (Boulder), and the earned badges are contiguous from badge 1 -- once a
    // badge is missing, every later badge is missing too (no random gaps).
    QVERIFY(b->badgeAt(0)); // Boulder always granted (at least one badge)
    {
      bool seenMissing = false;
      int earned = 0;
      for(int i = 0; i < int(maxBadges); i++) {
        if(b->badgeAt(i)) {
          QVERIFY2(!seenMissing, "badges are not contiguous from badge 1");
          earned++;
        } else {
          seenMissing = true;
        }
      }
      QVERIFY(earned >= 1 && earned <= int(maxBadges));
    }

    QVERIFY2(isStarter(b->playerStarter),
             qPrintable(QStringLiteral("starter %1 is not one of the 3 starters").arg(b->playerStarter)));
  }
}

void TestRandomizer::pokedexRandomize_countsStaySane()
{
  SaveFile sf; loadInto(sf, m_orig);
  PlayerPokedex* dex = sf.dataExpanded->player->pokedex;

  for(int iter = 0; iter < 20; iter++) {
    dex->randomize();
    QVERIFY(dex->ownedCount() >= 0 && dex->ownedCount() <= maxPokedex);
    QVERIFY(dex->seenCount() >= 0 && dex->seenCount() <= maxPokedex);
  }
}

void TestRandomizer::newStarterPokemon_isAValidStarter()
{
  SaveFile sf; loadInto(sf, m_orig);
  PlayerBasics* basics = sf.dataExpanded->player->basics;

  for(int iter = 0; iter < 20; iter++) {
    PokemonBox* mon = PokemonBox::newPokemon(PokemonRandom::Random_Starters3, basics);
    QVERIFY(mon != nullptr);
    QVERIFY2(mon->isValidBool(), "new starter is not a valid Pokedex species");
    QVERIFY2(isStarter(mon->species),
             qPrintable(QStringLiteral("rolled species %1 is not a starter").arg(mon->species)));
    QVERIFY(mon->level >= 1 && mon->level <= 100);
    delete mon;
  }
}

void TestRandomizer::fullRandomizeExpansion_runsAndHoldsInvariants()
{
  // Map/area randomization is disabled (maps WIP) in SaveFileExpanded::randomize(),
  // so the whole-tree randomize now runs end-to-end without crashing. Run it several
  // times and assert the trainer-basics invariants hold on the fully-randomized tree.
  SaveFile sf; loadInto(sf, m_orig);

  for(int iter = 0; iter < 10; iter++) {
    sf.randomizeExpansion();

    PlayerBasics* b = sf.dataExpanded->player->basics;
    QVERIFY2(b->money >= 100 && b->money <= 6000,
             qPrintable(QStringLiteral("money out of range: %1").arg(b->money)));
    QVERIFY(b->coins >= 0 && b->coins <= 100);
    // Linear badge progression: always at least the first badge, contiguous from badge 1.
    QVERIFY(b->badgeAt(0));
    for(int i = 1; i < int(maxBadges); i++)
      QVERIFY2(!b->badgeAt(i) || b->badgeAt(i - 1), "badges not contiguous from badge 1");
    QVERIFY(isStarter(b->playerStarter));
    QVERIFY(!b->getPlayerName().isEmpty());
  }
}

QTEST_GUILESS_MAIN(TestRandomizer)
#include "tst_randomizer.moc"
