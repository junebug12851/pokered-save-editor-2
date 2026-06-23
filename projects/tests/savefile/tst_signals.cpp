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
 * @file tst_signals.cpp
 * @brief Signal/slot tests (QSignalSpy). The rest of the savefile suite asserts
 *        VALUES; this asserts the *notification layer*: that mutating a fragment
 *        emits exactly the right change signal (count + that no-ops still honor
 *        their documented emit contract), AND that the internal `connect()` wiring
 *        actually fires (e.g. PokemonMove's ppUpChanged -> ppCapChanged). The
 *        notification layer is what every QML binding depends on, yet a value-only
 *        test passes even if a signal is never emitted (binding silently stale).
 *
 *        Headless / guiless (QObject signals need no GUI). Covers:
 *          - PokemonMove: maxPpUp/raisePpUp/lowerPpUp/resetPpUp/restorePP emit, with
 *            value clamping, the always-emit-even-at-cap contract, and the
 *            ppUpChanged->ppCapChanged / ppChanged->ppCapChanged slot wiring.
 *          - PlayerBasics: badgeSet + the targeted randomizers emit exactly their
 *            one signal; reset() fans out to all the field signals.
 */

#include <QtTest>
#include <QSignalSpy>

#include <pse-db/db.h>
#include <pse-db/pokemon.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

class TestSignals : public QObject
{
  Q_OBJECT

private:
  SaveFile m_sf; // blank save -> a valid PlayerBasics for OT/trade context

  PlayerBasics* basics() { return m_sf.dataExpanded->player->basics; }

  PokemonBox* makeMon(const QString& species)
  {
    PokemonDBEntry* e = PokemonDB::inst()->getIndAt(species);
    Q_ASSERT(e != nullptr);
    return PokemonBox::newPokemon(e, basics());
  }

private slots:
  void initTestCase();

  void move_ppUpControlsEmitPpUpChanged();
  void move_ppUpChangedWiredToPpCapChanged();
  void move_controlsAlwaysEmitEvenAtBounds();
  void move_restorePpEmitsPpChanged();

  void basics_badgeSetEmitsBadgesChanged();
  void basics_targetedRandomizersEmitExactlyOne();
  void basics_resetFansOutToAllFieldSignals();
};

void TestSignals::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  QVERIFY(PokemonDB::inst()->getIndAt(QStringLiteral("Bulbasaur")) != nullptr);
}

// --- PokemonMove -----------------------------------------------------------

void TestSignals::move_ppUpControlsEmitPpUpChanged()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  PokemonMove* m = p->movesAt(0);

  m->resetPpUp();                       // known baseline: 0
  QCOMPARE(m->ppUp, 0);

  QSignalSpy spy(m, &PokemonMove::ppUpChanged);
  QVERIFY(spy.isValid());

  m->maxPpUp();
  QCOMPARE(m->ppUp, 3);
  QCOMPARE(spy.count(), 1);

  m->lowerPpUp();
  QCOMPARE(m->ppUp, 2);
  QCOMPARE(spy.count(), 2);

  m->raisePpUp();
  QCOMPARE(m->ppUp, 3);
  QCOMPARE(spy.count(), 3);

  m->resetPpUp();
  QCOMPARE(m->ppUp, 0);
  QCOMPARE(spy.count(), 4);

  delete p;
}

void TestSignals::move_ppUpChangedWiredToPpCapChanged()
{
  // The ctor connects ppUpChanged -> ppCapChanged (PP cap depends on PP-Ups). A
  // QSignalSpy on the DOWNSTREAM signal proves the slot wiring actually fires, not
  // just that the direct signal emits.
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  PokemonMove* m = p->movesAt(0);

  QSignalSpy capSpy(m, &PokemonMove::ppCapChanged);
  QVERIFY(capSpy.isValid());

  m->maxPpUp();                          // emits ppUpChanged -> chained ppCapChanged
  QVERIFY2(capSpy.count() >= 1, "ppUpChanged was not wired through to ppCapChanged");

  // ppChanged is also wired to ppCapChanged.
  const int before = capSpy.count();
  m->restorePP();                        // emits ppChanged -> chained ppCapChanged
  QVERIFY(capSpy.count() > before);

  delete p;
}

void TestSignals::move_controlsAlwaysEmitEvenAtBounds()
{
  // Contract: raise/lower notify on every call even when the value is already at a
  // bound (a no-op change) -- so a QML control re-reads and re-clamps. Guards against
  // an "optimization" that skips the emit and leaves a binding stale.
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  PokemonMove* m = p->movesAt(0);

  m->maxPpUp();                          // ppUp == 3 (the upper bound)
  QSignalSpy spy(m, &PokemonMove::ppUpChanged);
  m->raisePpUp();                        // already maxed -> value unchanged...
  QCOMPARE(m->ppUp, 3);
  QCOMPARE(spy.count(), 1);              // ...but still emits

  m->resetPpUp();                        // ppUp == 0 (the lower bound)
  QSignalSpy spy2(m, &PokemonMove::ppUpChanged);
  m->lowerPpUp();                        // already at 0 -> value unchanged...
  QCOMPARE(m->ppUp, 0);
  QCOMPARE(spy2.count(), 1);             // ...but still emits

  delete p;
}

void TestSignals::move_restorePpEmitsPpChanged()
{
  PokemonBox* p = makeMon(QStringLiteral("Bulbasaur"));
  PokemonMove* m = p->movesAt(0);

  QSignalSpy spy(m, &PokemonMove::ppChanged);
  QVERIFY(spy.isValid());
  m->restorePP();
  QVERIFY(spy.count() >= 1);

  delete p;
}

// --- PlayerBasics ----------------------------------------------------------

void TestSignals::basics_badgeSetEmitsBadgesChanged()
{
  PlayerBasics* b = basics();
  QSignalSpy spy(b, &PlayerBasics::badgesChanged);
  QVERIFY(spy.isValid());

  b->badgeSet(0, true);
  QCOMPARE(spy.count(), 1);
  QVERIFY(b->badgeAt(0));

  b->badgeSet(0, false);                 // setting a flag (even to a new value)
  QCOMPARE(spy.count(), 2);
  QVERIFY(!b->badgeAt(0));
}

void TestSignals::basics_targetedRandomizersEmitExactlyOne()
{
  PlayerBasics* b = basics();

  QSignalSpy coins(b, &PlayerBasics::coinsChanged);
  QSignalSpy money(b, &PlayerBasics::moneyChanged);
  QSignalSpy id(b, &PlayerBasics::playerIDChanged);

  b->randomizeCoins();
  b->randomizeMoney();
  b->randomizeID();

  // Each targeted randomizer touches exactly its own field's signal once.
  QCOMPARE(coins.count(), 1);
  QCOMPARE(money.count(), 1);
  QCOMPARE(id.count(), 1);
}

void TestSignals::basics_resetFansOutToAllFieldSignals()
{
  PlayerBasics* b = basics();

  QSignalSpy name(b, &PlayerBasics::playerNameChanged);
  QSignalSpy id(b, &PlayerBasics::playerIDChanged);
  QSignalSpy money(b, &PlayerBasics::moneyChanged);
  QSignalSpy coins(b, &PlayerBasics::coinsChanged);
  QSignalSpy badges(b, &PlayerBasics::badgesChanged);
  QSignalSpy starter(b, &PlayerBasics::playerStarterChanged);

  b->reset();                            // blanks every field -> notifies every field

  QVERIFY(name.count() >= 1);
  QVERIFY(id.count() >= 1);
  QVERIFY(money.count() >= 1);
  QVERIFY(coins.count() >= 1);
  QVERIFY(badges.count() >= 1);
  QVERIFY(starter.count() >= 1);
}

QTEST_GUILESS_MAIN(TestSignals)
#include "tst_signals.moc"
