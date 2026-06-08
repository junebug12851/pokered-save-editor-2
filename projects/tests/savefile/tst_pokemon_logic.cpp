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
 * @file tst_pokemon_logic.cpp
 * @brief Coverage-climbing: behavioural tests for PokemonBox's computed
 *        properties and editing slots (the richest object in the tree) -- DV/EV
 *        max/reset, level max/maxOut, Pokecenter heal, evolve/de-evolve, shiny
 *        toggle, derived stats, and nature. Exercises a large slice of
 *        pokemonbox.cpp that the round-trip tests don't.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/pokemon.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

class TestPokemonLogic : public QObject
{
  Q_OBJECT

private:
  SaveFile m_sf; // blank save; supplies a valid PlayerBasics for OT context

  PokemonBox* makeMon(const QString& species)
  {
    PokemonDBEntry* e = PokemonDB::inst()->getIndAt(species);
    return PokemonBox::newPokemon(e, m_sf.dataExpanded->player->basics);
  }
  int ind(const QString& species) { return PokemonDB::inst()->getIndAt(species)->ind; }

private slots:
  void initTestCase();
  void dvEv_maxAndReset();
  void level_maxAndMaxOut();
  void heal_fillsHpAndClearsStatus();
  void evolve_andDeEvolve();
  void shiny_makeAndUnmake();
  void computedStats_positiveAndBounded();
  void nature_inEnumRange();
};

void TestPokemonLogic::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(PokemonDB::inst()->getIndAt(QStringLiteral("Bulbasaur")) != nullptr);
}

void TestPokemonLogic::dvEv_maxAndReset()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  p->maxDVs();
  QVERIFY(p->isMaxDVs());
  for(int i = 0; i < p->dvCount(); i++) QCOMPARE(p->dvAt(i), 15);

  p->resetDVs();
  QVERIFY(p->isMinDVs());

  p->maxEVs();
  QVERIFY(p->isMaxEVs());

  p->resetEVs();
  QVERIFY(p->isMinEvs());

  delete p;
}

void TestPokemonLogic::level_maxAndMaxOut()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  p->maxLevel();
  QCOMPARE(p->level, 100);
  QVERIFY(p->isMaxLevel());

  p->maxOut();
  QVERIFY(p->isMaxedOut());
  QVERIFY(p->isMaxDVs());
  QVERIFY(p->isMaxEVs());

  delete p;
}

void TestPokemonLogic::heal_fillsHpAndClearsStatus()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);
  p->maxLevel();
  p->update(true, false, false, false, false); // recompute stats (reset HP)

  p->hp = 1;
  p->status = 8; // some status condition
  p->heal();

  // heal()'s contract: HP back to max + status cleared (+ PP refilled). Note
  // isHealed() is stricter (isMaxHp && !afflicted && isMaxPP), and a freshly-built
  // mon has empty move slots that aren't "max PP", so assert the contract directly.
  QVERIFY(p->isMaxHp());
  QCOMPARE(p->status, 0);
  QCOMPARE(p->hp, p->hpStat());

  delete p;
}

void TestPokemonLogic::evolve_andDeEvolve()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);
  QVERIFY(p->hasEvolution());

  p->evolve();
  QCOMPARE(p->species, ind(QStringLiteral("Ivysaur")));
  QVERIFY(p->hasDeEvolution());

  p->deEvolve();
  QCOMPARE(p->species, ind(QStringLiteral("Bulbasaur")));

  delete p;
}

void TestPokemonLogic::shiny_makeAndUnmake()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  p->makeShiny();
  QVERIFY2(p->isShiny(), "makeShiny() did not produce a shiny DV combination");

  p->unmakeShiny();
  QVERIFY2(!p->isShiny(), "unmakeShiny() left it shiny");

  delete p;
}

void TestPokemonLogic::computedStats_positiveAndBounded()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);
  p->maxLevel();
  p->maxDVs();
  p->maxEVs();
  p->update(true, false, false, false, false);

  // Gen 1 stats are well under 1000; just assert sane positive values.
  QVERIFY(p->hpStat()  > 0 && p->hpStat()  < 1000);
  QVERIFY(p->atkStat() > 0 && p->atkStat() < 1000);
  QVERIFY(p->defStat() > 0 && p->defStat() < 1000);
  QVERIFY(p->spdStat() > 0 && p->spdStat() < 1000);
  QVERIFY(p->spStat()  > 0 && p->spStat()  < 1000);

  delete p;
}

void TestPokemonLogic::nature_inEnumRange()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);
  const int n = p->getNature();
  QVERIFY2(n >= 0 && n <= 24, qPrintable(QStringLiteral("nature %1 out of 0..24").arg(n)));
  delete p;
}

QTEST_GUILESS_MAIN(TestPokemonLogic)
#include "tst_pokemon_logic.moc"
