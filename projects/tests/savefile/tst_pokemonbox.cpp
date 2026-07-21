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
 * @file tst_pokemonbox.cpp
 * @brief Coverage-climbing: the PokemonBox/PokemonMove logic the existing
 *        pokemon tests don't reach. tst_pokemon.cpp covers field round-trips and
 *        tst_pokemon_logic.cpp covers DV/EV/level/heal/evolve/shiny/stats/nature;
 *        this fills the remaining ~314 untested lines of pokemonbox.cpp:
 *
 *        - PokemonMove: PP-Up controls (max/raise/lower/reset, with clamping),
 *          restorePP, changeMove, moveType (valid + glitch + empty), isInvalid,
 *          allValidMoves / validMovesLeft / isDuplicateMove, correctMove.
 *        - PokemonBox: the constrained randomize() (the big ~70-line method),
 *          newPokemon across all four PokemonRandom_ scopes, resetExp,
 *          expLevelRangePercent, setNature (incl. the level-range clamp),
 *          hasTradeStatus, changeOtData / changeTrade (both directions, idempotent
 *          no-op path), cleanupMoves / correctMoves / update(correctMoves),
 *          changeMove(ind,...), the manual* UI hooks, reRollEVs / maxPpUps,
 *          dexNum / speciesName, isPokemonReset / isCorrected, isBoxMon, and the
 *          invalid-mon (species 0) safe-default guard paths.
 *
 *        All paths are headless-safe (no Maps/sprite linkage). Random-driven
 *        methods are asserted on their invariants, not exact values.
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/pokemon.h>
#include <pse-db/moves.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

class TestPokemonBox : public QObject
{
  Q_OBJECT

private:
  SaveFile m_sf; // blank save; supplies a valid PlayerBasics for OT/trade context

  PlayerBasics* basics() { return m_sf.dataExpanded->player->basics; }

  // A valid, game-accurate mon (species + initial moves + correct stats), built
  // the same way newPokemon does. Caller owns it.
  PokemonBox* makeMon(const QString& species)
  {
    PokemonDBEntry* e = PokemonDB::inst()->getIndAt(species);
    Q_ASSERT(e != nullptr);
    return PokemonBox::newPokemon(e, basics());
  }

private slots:
  void initTestCase();

  // PokemonMove
  void move_ppUpControlsClampAndRestore();
  void move_changeMoveAndType();
  void move_validMovesAndDuplicates();
  void move_correctMoveRepairs();

  // PokemonBox editing / logic
  void box_resetExpAndLevelRangePercent();
  void box_setNatureKeepsLevelRange();
  void box_tradeStatusAndOtData();
  void box_cleanupAndCorrectMoves();
  void box_changeMoveByIndexAndManualHooks();
  void box_reorderMove();
  void box_deleteMoveAndClearButFirst();
  void box_correctMoveAtCompacts();
  void move_ppAndPpUpIndependent();
  void box_correctTypesResetsToSpeciesDefault();
  void box_singleTypeCanonicalForm();
  void box_copyFromCopiesEachStatExp();
  void box_reRollEvsAndMaxPpUps();
  void box_dexNumAndSpeciesName();
  void box_resetMakesItPokemonResetAndCorrected();
  void box_healedWithFewerThanFourMoves();
  void box_isBoxMon();

  // The big constrained randomizer + the static newPokemon scopes
  void box_randomizeHoldsInvariants();
  void newPokemon_everyScopeProducesAMon();

  // Invalid (species 0) mon: every guard path returns its safe default
  void invalidMon_safeDefaults();
};

void TestPokemonBox::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(PokemonDB::inst()->getIndAt(QStringLiteral("Bulbasaur")) != nullptr);
  QVERIFY(basics() != nullptr);
}

void TestPokemonBox::move_ppUpControlsClampAndRestore()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  // movesAt() is the GC-wrapped public getter QML uses.
  PokemonMove* m = p->movesAt(0);
  QVERIFY(m != nullptr);
  QVERIFY(m->moveID > 0);          // reset mon has its initial move here

  m->resetPpUp();
  QCOMPARE(m->ppUp, 0);
  QVERIFY(!m->isMaxPpUps());

  m->raisePpUp();
  QCOMPARE(m->ppUp, 1);

  m->lowerPpUp();
  QCOMPARE(m->ppUp, 0);
  m->lowerPpUp();                  // already 0 -> clamped, stays 0
  QCOMPARE(m->ppUp, 0);

  m->maxPpUp();
  QCOMPARE(m->ppUp, 3);
  QVERIFY(m->isMaxPpUps());
  m->raisePpUp();                  // already 3 -> clamped, stays 3
  QCOMPARE(m->ppUp, 3);

  // restorePP refills to the (PP-Up-boosted) cap.
  m->pp = 0;
  m->restorePP();
  QCOMPARE(m->pp, m->getMaxPP());
  QVERIFY(m->isMaxPP());

  delete p;
}

void TestPokemonBox::move_changeMoveAndType()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  PokemonMove* m = p->moves[0];
  const int validId = m->moveID;            // a real, non-glitch move
  QVERIFY(validId > 0);

  // A real move resolves a (non-empty, non-"Glitch") type name.
  const QString t = m->moveType();
  QVERIFY(!t.isEmpty());
  QVERIFY(t != QStringLiteral("Glitch"));
  QVERIFY(!m->isInvalid());

  // Empty slot -> empty type string, and is "invalid".
  m->changeMove(0, 0, 0);
  QCOMPARE(m->moveID, 0);
  QCOMPARE(m->moveType(), QString());
  QVERIFY(m->isInvalid());

  // A non-zero but out-of-range id is a glitch move.
  m->moveID = 250;                          // beyond the real move table
  m->moveIDChanged();
  if(m->isInvalid())                         // (guard: in case 250 ever becomes real)
    QCOMPARE(m->moveType(), QStringLiteral("Glitch"));

  // changeMove restores a clean, valid slot.
  m->changeMove(validId, 5, 1);
  QCOMPARE(m->moveID, validId);
  QCOMPARE(m->ppUp, 1);

  delete p;
}

void TestPokemonBox::move_validMovesAndDuplicates()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  PokemonMove* m0 = p->moves[0];
  QVERIFY(m0->moveID > 0);

  // The legal move pool for a real species is non-empty...
  QVector<int> all = m0->allValidMoves();
  QVERIFY(!all.isEmpty());

  // ...and "moves left" excludes ids the mon already uses.
  QVector<int> left = m0->validMovesLeft();
  for(int i = 0; i < 4; i++) {
    const int used = p->moves[i]->moveID;
    if(used > 0)
      QVERIFY2(!left.contains(used), "validMovesLeft still lists an already-known move");
  }

  // Duplicate detection across slots.
  p->moves[1]->changeMove(m0->moveID, 5, 0);
  QVERIFY(m0->isDuplicateMove());
  QVERIFY(p->moves[1]->isDuplicateMove());

  delete p;
}

void TestPokemonBox::move_correctMoveRepairs()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  // Force a duplicate, then correct the second slot: it must no longer collide.
  const int id = p->moves[0]->moveID;
  QVERIFY(id > 0);
  p->moves[1]->changeMove(id, 5, 0);
  QVERIFY(p->moves[1]->isDuplicateMove());

  p->moves[1]->correctMove();
  // After correction the two slots no longer hold the same id (it's either zeroed
  // or replaced with a different legal move).
  QVERIFY(p->moves[1]->moveID != id || p->moves[0]->moveID != id ? true
          : p->moves[1]->moveID == 0);
  QVERIFY(!(p->moves[0]->moveID == id && p->moves[1]->moveID == id));

  delete p;
}

void TestPokemonBox::box_resetExpAndLevelRangePercent()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  p->level = 30;
  p->levelChanged();

  // Inflate exp well past the level's window, then resetExp pins it to the start.
  p->exp = p->expLevelRangeEnd() + 100000u;
  p->expChanged();
  p->resetExp();
  QCOMPARE(p->exp, p->expLevelRangeStart());

  // Percent through the level is in [0,1] for a normal level...
  const float pct = p->expLevelRangePercent();
  QVERIFY(pct >= 0.0f && pct <= 1.0f);

  // Regression (clang-tidy bugprone-integer-division): mid-window must be a genuine
  // FRACTION. The old `curExp / expEnd` divided two var32s in INTEGER math before
  // widening to the float return, so it was truncated to 0.0 everywhere except the
  // very top of the level. Put exp at the window midpoint and require ~0.5.
  p->exp = p->expLevelRangeStart()
           + (p->expLevelRangeEnd() - p->expLevelRangeStart()) / 2;
  p->expChanged();
  const float midPct = p->expLevelRangePercent();
  QVERIFY2(midPct > 0.4f && midPct < 0.6f,
           qPrintable(QStringLiteral("midPct=%1 (would be 0 under the old integer "
                                     "division)").arg(midPct)));

  // ...and is exactly 1 at level 100.
  p->level = 100;
  p->levelChanged();
  QCOMPARE(p->expLevelRangePercent(), 1.0f);

  delete p;
}

void TestPokemonBox::box_setNatureKeepsLevelRange()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  p->level = 50;
  p->levelChanged();
  p->update(false, true);          // exp -> levelToExp(50) (the level-window start)

  const int cur = p->getNature();
  QVERIFY(cur >= 0 && cur <= 24);
  const int target = (cur + 7) % 25;   // guaranteed different from cur

  p->setNature(target);
  QCOMPARE(p->getNature(), target);    // value applied, mod 25

  // The clamp keeps the mon inside its level window (it must not have changed level).
  QVERIFY(p->exp >= p->expLevelRangeStart());
  QVERIFY(p->exp <= p->expLevelRangeEnd());

  // Setting the same nature again is a no-op (the early-return branch).
  p->setNature(target);
  QCOMPARE(p->getNature(), target);

  delete p;
}

void TestPokemonBox::box_tradeStatusAndOtData()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  // Adopt the player's OT -> not traded.
  p->changeOtData(true, basics());
  QCOMPARE(p->otName, basics()->playerName);
  QCOMPARE(p->otID,  basics()->playerID);
  QVERIFY(!p->hasTradeStatus(basics()));

  // Calling remove again is idempotent (no-op branch) -- still owned.
  p->changeOtData(true, basics());
  QVERIFY(!p->hasTradeStatus(basics()));

  // Randomize OT -> traded (force a guaranteed difference to avoid RNG collision).
  p->otName = basics()->playerName + QStringLiteral("X");
  p->otNameChanged();
  QVERIFY(p->hasTradeStatus(basics()));

  // changeTrade(false) randomizes name + OT together (gives traded status).
  p->changeTrade(false);
  QVERIFY(p->hasTradeStatus(basics()));

  // remove path with a null basics is a safe no-op (early return).
  const QString before = p->otName;
  p->changeOtData(true, nullptr);
  QCOMPARE(p->otName, before);

  delete p;
}

void TestPokemonBox::box_cleanupAndCorrectMoves()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);
  QVERIFY(p->movesCount() >= 1);

  // Punch a hole at slot 0 while keeping a later real move, then compact.
  const int keep = p->moves[1]->moveID > 0 ? p->moves[1]->moveID : p->moves[0]->moveID;
  if(p->moves[1]->moveID > 0) {
    p->moves[0]->moveID = 0; p->moves[0]->moveIDChanged();
    p->cleanupMoves();
    QVERIFY2(p->moves[0]->moveID > 0, "cleanupMoves did not compact the gap");
    QCOMPARE(p->moves[0]->moveID, keep);
  }

  // Duplicate two slots, then correctMoves must not leave both equal.
  const int id = p->moves[0]->moveID;
  p->moves[1]->changeMove(id, 5, 0);
  p->correctMoves();
  QVERIFY(!(p->moves[0]->moveID == id && p->moves[1]->moveID == id));

  // update(correctMoves=true) runs correct+cleanup without crashing.
  p->update(true, true, true, true, true);
  QVERIFY(p->movesCount() <= p->movesMax());

  delete p;
}

void TestPokemonBox::box_changeMoveByIndexAndManualHooks()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  const int validId = p->moves[0]->moveID;
  p->changeMove(2, validId, 7, 1);
  QCOMPARE(p->moves[2]->moveID, validId);
  QCOMPARE(p->moves[2]->ppUp, 1);

  // Bulbasaur is dual-type (Grass/Poison); capture its real second type.
  const int dualType2 = p->type2;
  QVERIFY(dualType2 != 0xFF);
  QVERIFY(dualType2 != p->type1);

  // manualLevelChanged() re-runs update(resetHp, resetExp) WITHOUT resetType.
  // Regression guard for the type2-clobber fix: the second type must SURVIVE
  // (it used to be overwritten with type1), HP/EXP are recomputed for the new
  // level, and the mon still reads as internally corrected.
  p->level = 25;
  p->levelChanged();
  p->manualLevelChanged();
  QCOMPARE(p->type2, dualType2);                        // second type preserved (#1 fix)
  QCOMPARE(p->hp, p->hpStat());
  QCOMPARE(p->exp, p->levelToExp());
  QVERIFY(p->isCorrected());

  // Swap species and re-derive everything via the species hook (resetType too).
  PokemonDBEntry* charmander = PokemonDB::inst()->getIndAt(QStringLiteral("Charmander"));
  QVERIFY(charmander != nullptr);
  p->species = charmander->ind;
  p->speciesChanged();
  p->manualSpeciesChanged();
  QCOMPARE(p->dexNum(), *charmander->pokedex);          // raw (0-indexed) pokedex field
  QCOMPARE(p->type1, charmander->toType1->ind);         // primary type re-derived
  QCOMPARE(p->catchRate, (int)*charmander->catchRate);  // catch rate reset
  QCOMPARE(p->hp, p->hpStat());                         // HP recomputed
  QCOMPARE(p->exp, p->levelToExp());                    // EXP pinned to the level
  QVERIFY(p->isCorrected());                            // single-type mon reads corrected (#2 fix)

  delete p;
}

// reorderMove() drag-reorders the FILLED move slots (id/pp/ppUp travel together),
// leaving the fixed slot objects in place and keeping the move list compact. The
// drop-slot convention matches the storage drags: toIndex inserts BEFORE that slot,
// toIndex == movesCount appends after the last move.
void TestPokemonBox::box_reorderMove()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  // Give the mon a known, distinct 4-move set with distinct PP so we can track
  // each move (and prove PP rides along with its move).
  p->changeMove(0, 10, 11, 0);
  p->changeMove(1, 20, 21, 1);
  p->changeMove(2, 30, 31, 2);
  p->changeMove(3, 40, 41, 3);

  // Move slot 0 to the end (toIndex == 4 appends): {10,20,30,40} -> {20,30,40,10}.
  p->reorderMove(0, 4);
  QCOMPARE(p->moves[0]->moveID, 20);
  QCOMPARE(p->moves[1]->moveID, 30);
  QCOMPARE(p->moves[2]->moveID, 40);
  QCOMPARE(p->moves[3]->moveID, 10);
  QCOMPARE(p->moves[3]->pp, 11);     // PP rode along with move 10
  QCOMPARE(p->moves[3]->ppUp, 0);

  // Move the last move to the front (insert before slot 0): {20,30,40,10} -> {10,20,30,40}.
  p->reorderMove(3, 0);
  QCOMPARE(p->moves[0]->moveID, 10);
  QCOMPARE(p->moves[1]->moveID, 20);
  QCOMPARE(p->moves[2]->moveID, 30);
  QCOMPARE(p->moves[3]->moveID, 40);

  // Insert-before in the middle: move slot 0 before slot 2 -> {20,10,30,40}.
  p->reorderMove(0, 2);
  QCOMPARE(p->moves[0]->moveID, 20);
  QCOMPARE(p->moves[1]->moveID, 10);
  QCOMPARE(p->moves[2]->moveID, 30);
  QCOMPARE(p->moves[3]->moveID, 40);

  // Only the filled prefix reorders + stays compact. With two moves, dragging an
  // EMPTY slot (index 2) is a no-op, and reordering the two filled keeps the two
  // empty slots parked at the bottom.
  p->clearMoves();
  p->changeMove(0, 55, 5, 0);
  p->changeMove(1, 66, 6, 0);
  p->reorderMove(2, 0);                 // empty source -> no-op
  QCOMPARE(p->moves[0]->moveID, 55);
  QCOMPARE(p->moves[1]->moveID, 66);
  p->reorderMove(1, 0);                 // swap the two filled moves
  QCOMPARE(p->moves[0]->moveID, 66);
  QCOMPARE(p->moves[1]->moveID, 55);
  QCOMPARE(p->moves[2]->moveID, 0);     // empties still parked
  QCOMPARE(p->moves[3]->moveID, 0);

  delete p;
}

// deleteMoveAt() clears a slot and compacts (no gap); clearMovesButFirst() keeps
// only the leading move. Both keep the move list compact (empties at the bottom).
void TestPokemonBox::box_deleteMoveAndClearButFirst()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  p->changeMove(0, 10, 11, 0);
  p->changeMove(1, 20, 21, 0);
  p->changeMove(2, 30, 31, 0);
  p->changeMove(3, 40, 41, 0);

  // Delete the middle move -> the later moves slide up, no gap, last slot empties.
  p->deleteMoveAt(1);
  QCOMPARE(p->moves[0]->moveID, 10);
  QCOMPARE(p->moves[1]->moveID, 30);
  QCOMPARE(p->moves[2]->moveID, 40);
  QCOMPARE(p->moves[3]->moveID, 0);
  QCOMPARE(p->movesCount(), 3);

  // Out-of-range delete is a no-op.
  p->deleteMoveAt(9);
  QCOMPARE(p->movesCount(), 3);

  // clearMovesButFirst keeps only the leading move.
  p->clearMovesButFirst();
  QCOMPARE(p->moves[0]->moveID, 10);
  QCOMPARE(p->moves[1]->moveID, 0);
  QCOMPARE(p->moves[2]->moveID, 0);
  QCOMPARE(p->moves[3]->moveID, 0);
  QCOMPARE(p->movesCount(), 1);

  delete p;
}

// correctMoveAt() validates a slot AND compacts: a duplicate/invalid move that
// correctMove clears must not leave a gap -- the later moves slide up.
void TestPokemonBox::box_correctMoveAtCompacts()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  const int a = p->moves[0]->moveID;
  const int b = p->moves[1]->moveID;
  if(a <= 0 || b <= 0 || a == b)
    QSKIP("need two distinct initial moves for this test");

  // slot0 = A, slot1 = duplicate of A, slot2 = B.
  p->changeMove(1, a, 5, 0);
  p->changeMove(2, b, 5, 0);
  QVERIFY(p->moves[1]->isDuplicateMove());

  // Validate the duplicate slot: it's cleared, and B slides up -- no gap.
  p->correctMoveAt(1);
  QCOMPARE(p->moves[0]->moveID, a);
  QCOMPARE(p->moves[1]->moveID, b);   // slid up from slot 2
  QCOMPARE(p->moves[2]->moveID, 0);
  QCOMPARE(p->movesCount(), 2);

  delete p;
}

// PP and PP-Ups are independent fields: the "set to max" actions and direct edits
// each touch ONLY their own field. (The Moves tab's PP / PP-Ups views must never
// cross-write -- this is the model-level guarantee the UI relies on.)
void TestPokemonBox::move_ppAndPpUpIndependent()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  PokemonMove* m = p->moves[0];
  QVERIFY(m->moveID > 0);

  // A known starting point: low PP, no PP-Ups.
  m->changeMove(m->moveID, 1, 0);
  QCOMPARE(m->pp, 1);
  QCOMPARE(m->ppUp, 0);

  // restorePP() fills PP to the cap and leaves PP-Ups alone.
  m->restorePP();
  QCOMPARE(m->pp, m->getMaxPP());
  QCOMPARE(m->ppUp, 0);

  // maxPpUp() sets PP-Ups to 3 and leaves the current PP value alone (it does NOT
  // auto-refill PP, even though the cap rises).
  const int ppBefore = m->pp;
  m->maxPpUp();
  QCOMPARE(m->ppUp, 3);
  QCOMPARE(m->pp, ppBefore);

  // Editing one field directly never disturbs the other.
  m->pp = 5;          m->ppChanged();
  QCOMPARE(m->ppUp, 3);
  m->ppUp = 1;        m->ppUpChanged();
  QCOMPARE(m->pp, 5);

  delete p;
}

// correctTypes() must reset a tampered typing back to the species' DB defaults.
// Charizard is Fire(20)/Flying(2) -- asserted against the literal type ids, NOT
// chz->toType1->ind (that would be tautological with what correctTypes reads).
void TestPokemonBox::box_correctTypesResetsToSpeciesDefault()
{
  const int FIRE = 20, FLYING = 2, GHOST = 8, FIGHTING = 1;

  PokemonBox* p = makeMon(QStringLiteral("Charizard"));
  QVERIFY(p != nullptr);

  // A fresh Charizard should already carry its real typing (sanity-checks the
  // species->type deep-link itself, not just correctTypes).
  QCOMPARE(p->type1, FIRE);
  QCOMPARE(p->type2, FLYING);

  // Tamper the typing (Ghost / Fighting), then correct it back.
  p->type1 = GHOST;
  p->type2 = FIGHTING;
  p->type2Explicit = true;
  p->correctTypes();

  QCOMPARE(p->type1, FIRE);
  QCOMPARE(p->type2, FLYING);

  delete p;
}

// The resolved type-2 "single truth" (2026-07-09, grounded in the pokered
// disassembly: base_stats store a single type as duplicate-of-type1, e.g.
// Charmander `db FIRE, FIRE`, and 0xFF is not a valid type). Internally a single
// type is the 0xFF sentinel; type2Explicit preserves a literally-loaded 0xFF for
// byte fidelity, but ANY editor (re)generation must drop it so the single type
// serialises as the canonical duplicate-of-type1, never a stray 0xFF.
void TestPokemonBox::box_singleTypeCanonicalForm()
{
  const int FIRE = 20;

  // Charmander is single-type Fire. A freshly built mon holds the single type as
  // the internal 0xFF sentinel with type2Explicit false -> save() writes the
  // duplicate-of-type1 form the game uses.
  PokemonBox* p = makeMon(QStringLiteral("Charmander"));
  QVERIFY(p != nullptr);
  QCOMPARE(p->type1, FIRE);
  QCOMPARE(p->type2, 0xFF);
  QVERIFY2(!p->type2Explicit,
           "a generated single type must be canonical (writes duplicate-of-type1), not explicit 0xFF");

  // Simulate a hacked save that literally stored 0xFF (type2Explicit true), then
  // run an editor correction: correctTypes() must drop the stale fidelity flag so
  // the corrected single type serialises as the canonical duplicate, not 0xFF.
  p->type2Explicit = true;
  p->type2ExplicitChanged();
  p->correctTypes();
  QCOMPARE(p->type1, FIRE);
  QCOMPARE(p->type2, 0xFF);
  QVERIFY2(!p->type2Explicit,
           "correctTypes() must clear the load-fidelity flag so a single type writes duplicate-of-type1");

  // A species/type reset via update(resetType) must clear the flag too (same
  // canonical guarantee through the manual*/update path).
  p->type2Explicit = true;
  p->type2ExplicitChanged();
  p->update(false, false, /*resetType=*/true, false);
  QVERIFY2(!p->type2Explicit,
           "update(resetType) must clear the load-fidelity flag for a single type");

  delete p;
}

// Regression guard for the copyFrom() stat-exp typo: Speed's stat-exp (spdExp) was
// copied from the SOURCE's Special (spExp), silently overwriting Speed with Special.
// Each stat-exp channel must copy from its own counterpart.
void TestPokemonBox::box_copyFromCopiesEachStatExp()
{
  PokemonBox* src = makeMon(QStringLiteral("Bulbasaur"));
  PokemonBox* dst = makeMon(QStringLiteral("Charmander"));
  QVERIFY(src != nullptr && dst != nullptr);

  // Distinct values per channel so a cross-wired copy cannot accidentally pass.
  src->hpExp  = 0x1111;
  src->atkExp = 0x2222;
  src->defExp = 0x3333;
  src->spdExp = 0x4444;
  src->spExp  = 0x5555;

  dst->copyFrom(src);

  QCOMPARE(dst->hpExp,  0x1111);
  QCOMPARE(dst->atkExp, 0x2222);
  QCOMPARE(dst->defExp, 0x3333);
  QCOMPARE(dst->spdExp, 0x4444);   // Speed stat-exp must come from Speed, not Special
  QCOMPARE(dst->spExp,  0x5555);

  delete src;
  delete dst;
}

void TestPokemonBox::box_reRollEvsAndMaxPpUps()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  p->reRollEVs();                  // random, 0..0xFFFF each -- just must run + bound
  QVERIFY(p->hpExp  >= 0 && p->hpExp  <= 0xFFFF);
  QVERIFY(p->atkExp >= 0 && p->atkExp <= 0xFFFF);

  p->maxPpUps();
  QVERIFY(p->isMaxPpUps());

  // isMinEvs() means ALL stat-exp zero (not "any one is zero"). Regression guard
  // for the ||->&& fix: one EV non-zero with the rest at 0 must read as NOT min.
  p->resetEVs();
  QVERIFY(p->isMinEvs());
  p->atkExp = 1; p->atkExpChanged();
  QVERIFY2(!p->isMinEvs(), "isMinEvs() must require ALL stat-exp to be zero");

  delete p;
}

void TestPokemonBox::box_dexNumAndSpeciesName()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  // dexNum() returns the raw pokedex field, which is 0-indexed in the DB (the
  // +1 to a human dex number is applied at the UI layer -- see Pokedex.qml).
  QCOMPARE(p->dexNum(), 0);                  // Bulbasaur => index 0 (dex #1)
  QVERIFY(!p->speciesName().isEmpty());

  delete p;
}

void TestPokemonBox::box_resetMakesItPokemonResetAndCorrected()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);

  // Rough it up, then reset to baseline.
  p->maxLevel();
  p->maxEVs();
  p->resetPokemon();

  // resetPokemon() restores level 5, initial moves (base PP, 0 PP-Ups), zeroed
  // EVs, full heal, and re-derives all stats/types -> internally consistent and,
  // after the isPokemonReset() fix, correctly recognised as a reset baseline even
  // for a species with fewer than four initial moves.
  QCOMPARE(p->level, 5);
  QVERIFY(p->isMinEvs());
  QVERIFY(p->isCorrected());
  QVERIFY2(p->isPokemonReset(), "resetPokemon() output should read as a reset baseline (#3 fix)");

  delete p;
}

void TestPokemonBox::box_healedWithFewerThanFourMoves()
{
  // Regression guard: a real species with fewer than four moves (Bulbasaur), at
  // full HP/PP with no status, must read as max-PP and healed. Before the fix,
  // empty move slots counted as "not max PP", so such a mon was never isHealed()
  // -- a user-facing wrong result on the heal indicator.
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);
  QVERIFY(p->movesCount() < 4);     // Bulbasaur starts with fewer than 4 moves

  p->heal();                         // full HP + clear status + refill PP
  QVERIFY2(p->isMaxPP(), "empty move slots must not count against isMaxPP()");
  QVERIFY(p->isMaxHp());
  QVERIFY(!p->isAfflicted());
  QVERIFY2(p->isHealed(), "a <4-move mon at full HP/PP must read as healed");

  delete p;
}

void TestPokemonBox::box_isBoxMon()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  QVERIFY(p != nullptr);
  QVERIFY2(p->isBoxMon(), "a PokemonBox must report as a box mon");
  delete p;
}

void TestPokemonBox::box_randomizeHoldsInvariants()
{
  // The big constrained randomizer: a pokedex species, balanced level, fresh
  // moves, non-traded OT, whacky-but-valid types. Run several seeds.
  for(int iter = 0; iter < 8; iter++) {
    PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
    QVERIFY(p != nullptr);

    p->randomize(basics());

    QVERIFY2(p->isValidBool(), "randomize() produced a non-pokedex species");
    QVERIFY(p->level >= 5 && p->level <= 100);
    QVERIFY(!p->hasTradeStatus(basics()));   // randomize makes it non-traded

    // type1 is a real type id; type2 is either a real differing type or 0xFF.
    QVERIFY(p->type1 >= 0 && p->type1 != 0xFF);
    QVERIFY(p->type2 == 0xFF || p->type2 != p->type1);

    delete p;
  }
}

void TestPokemonBox::newPokemon_everyScopeProducesAMon()
{
  const PokemonRandom::PokemonRandom_ scopes[] = {
    PokemonRandom::Random_Starters3,
    PokemonRandom::Random_Starters,
    PokemonRandom::Random_Pokedex,
    PokemonRandom::Random_All
  };

  for(auto scope : scopes) {
    PokemonBox* p = PokemonBox::newPokemon(scope, basics());
    QVERIFY2(p != nullptr, "newPokemon returned null for a random scope");
    QVERIFY(p->species > 0);
    QVERIFY(p->level >= 5 && p->level <= 8);   // newPokemon levels are 5..8
    // The three constrained scopes always yield a real Pokedex species.
    if(scope != PokemonRandom::Random_All)
      QVERIFY2(p->isValidBool(), "a constrained scope produced a non-pokedex species");
    delete p;
  }
}

void TestPokemonBox::invalidMon_safeDefaults()
{
  // A bare PokemonBox is species 0 -> not a Pokedex entry. Every guarded accessor
  // must return its defined safe default rather than crash.
  PokemonBox p;                    // no savefile, no species
  QCOMPARE(p.species, 0);
  QVERIFY(!p.isValidBool());
  QVERIFY(p.isValid() == nullptr);

  QCOMPARE(p.dexNum(), -1);
  QCOMPARE(p.speciesName(), QString());
  QCOMPARE(p.levelToExp(50), 0u);
  QCOMPARE(p.hpStat(), 1);                 // documented invalid-mon HP floor
  QCOMPARE(p.nonHpStat(PokemonStats::Attack), 0);
  QVERIFY(!p.hasNickname());
  QVERIFY(!p.hasEvolution());
  QVERIFY(!p.hasDeEvolution());
  QVERIFY(p.isCorrected());                // record null -> "corrected" by definition
  QVERIFY(!p.isPokemonReset());

  // expLevelRange* fall back to the raw exp for an invalid mon.
  p.exp = 12345u;
  QCOMPARE(p.expLevelRangeStart(), 12345u);
  QCOMPARE(p.expLevelRangeEnd(), 12345u);
  QCOMPARE(p.expLevelRangePercent(), 0.0f);

  // resetExp on an invalid mon is a no-op (early return) -- exp untouched.
  p.resetExp();
  QCOMPARE(p.exp, 12345u);
}

QTEST_GUILESS_MAIN(TestPokemonBox)
#include "tst_pokemonbox.moc"
