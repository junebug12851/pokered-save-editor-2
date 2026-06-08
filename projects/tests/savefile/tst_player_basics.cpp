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
 * @file tst_player_basics.cpp
 * @brief Coverage for PlayerBasics methods the field round-trip tests don't reach:
 *        the badge accessors (count/at/set), toStarter() resolution, the individual
 *        randomizers (coins/money/id/starter), and -- most importantly -- the
 *        OT-rewrite machinery: getNonTradeMons() (incl. the null-file guard),
 *        fixNonTradeMons(), and fullSetPlayerName / fullSetPlayerId, which must
 *        update every owned mon's OT while leaving the value-unchanged no-op path
 *        a true no-op (the byte-fidelity guard that fixed the name-edit hang).
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/pokemon.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

using namespace pse_test;

class TestPlayerBasics : public QObject
{
  Q_OBJECT

private:
  QByteArray m_orig;

private slots:
  void initTestCase();
  void badges_countAtSet();
  void toStarter_resolvesRandomizedStarter();
  void individualRandomizers_stayInRange();
  void getNonTradeMons_nullFileIsEmpty();
  void fullSetName_noOpWhenUnchanged();
  void fullSetName_rewritesOwnedMonOtName();
  void fullSetId_rewritesOwnedMonOtId();
};

void TestPlayerBasics::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  m_orig = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(m_orig.size(), kSaveSize);
}

void TestPlayerBasics::badges_countAtSet()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* pb = sf.dataExpanded->player->basics;

  QCOMPARE(pb->badgeCount(), int(maxBadges));

  pb->badgeSet(3, true);
  QCOMPARE(pb->badgeAt(3), true);
  pb->badgeSet(3, false);
  QCOMPARE(pb->badgeAt(3), false);

  QCOMPARE(pb->getPlayerName(), pb->playerName);
  QCOMPARE(pb->getPlayerId(), pb->playerID);
}

void TestPlayerBasics::toStarter_resolvesRandomizedStarter()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* pb = sf.dataExpanded->player->basics;

  pb->randomizeStarter();                 // sets playerStarter to a real starter ind
  PokemonDBEntry* s = pb->toStarter();
  QVERIFY(s != nullptr);
  QCOMPARE(int(s->ind), pb->playerStarter);
}

void TestPlayerBasics::individualRandomizers_stayInRange()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* pb = sf.dataExpanded->player->basics;

  for(int n = 0; n < 40; n++) {
    pb->randomizeCoins();
    QVERIFY(pb->coins >= 0 && pb->coins < 9999);

    pb->randomizeMoney();
    QVERIFY(pb->money <= 999999u);

    pb->randomizeID();
    QVERIFY(pb->playerID >= 0 && pb->playerID <= 0xFFFF);

    pb->randomizeStarter();
    QVERIFY(pb->toStarter() != nullptr);
  }
}

// getNonTradeMons must bail to an empty list when there's no backing save
// (file == nullptr) rather than dereferencing it.
void TestPlayerBasics::getNonTradeMons_nullFileIsEmpty()
{
  PlayerBasics pb(nullptr);
  QVERIFY(pb.getNonTradeMons().isEmpty());
}

// Setting the name to its current value must be a true no-op: no rewrite of any
// owned mon's OT (the fidelity + anti-hang guard).
void TestPlayerBasics::fullSetName_noOpWhenUnchanged()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* pb = sf.dataExpanded->player->basics;

  const QString name = pb->playerName;
  auto owned = pb->getNonTradeMons();
  QVector<QString> otBefore;
  for(auto* m : owned) otBefore.append(m->otName);

  pb->fullSetPlayerName(name); // same value -> early return

  QCOMPARE(pb->playerName, name);
  for(int i = 0; i < owned.size(); i++)
    QCOMPARE(owned.at(i)->otName, otBefore.at(i)); // untouched
}

void TestPlayerBasics::fullSetName_rewritesOwnedMonOtName()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* pb = sf.dataExpanded->player->basics;

  auto owned = pb->getNonTradeMons();
  if(owned.isEmpty())
    QSKIP("fixture has no owned (non-trade) mons to rewrite");

  PokemonBox* mon = owned.first();
  QCOMPARE(mon->otName, pb->playerName); // precondition: owned == OT matches player

  pb->fullSetPlayerName(QStringLiteral("NEWOT"));

  QCOMPARE(pb->playerName, QStringLiteral("NEWOT"));
  QCOMPARE(mon->otName, QStringLiteral("NEWOT")); // owned mon's OT followed
}

void TestPlayerBasics::fullSetId_rewritesOwnedMonOtId()
{
  SaveFile sf; loadInto(sf, m_orig);
  auto* pb = sf.dataExpanded->player->basics;

  auto owned = pb->getNonTradeMons();
  if(owned.isEmpty())
    QSKIP("fixture has no owned (non-trade) mons to rewrite");

  PokemonBox* mon = owned.first();
  QCOMPARE(mon->otID, pb->playerID); // precondition

  const int newId = (pb->playerID + 1) & 0xFFFF; // guaranteed different
  pb->fullSetPlayerId(newId);

  QCOMPARE(pb->playerID, newId);
  QCOMPARE(mon->otID, newId); // owned mon's OT id followed
}

QTEST_GUILESS_MAIN(TestPlayerBasics)
#include "tst_player_basics.moc"
