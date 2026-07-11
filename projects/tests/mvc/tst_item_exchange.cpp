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
 * @file tst_item_exchange.cpp
 * @brief The Market Exchange's item<->item trading (ItemExchangeModel).
 *
 * Pins the economics, because they are the part a user can't verify by eye and the part
 * that touches their save. The load-bearing rule: the WHOLE trade is priced once (the
 * total value is rounded up to a whole number of the given item, and only that single
 * leftover is refunded), NOT once per step. Asking for 3 Fresh Water (3 x ₽200 = ₽600)
 * therefore costs exactly 2 Potions (2 x ₽300 = ₽600) and refunds NOTHING -- pricing each
 * Fresh Water separately would have taken 3 Potions and handed back ₽300 out of thin air.
 *
 * Also pins the dropdown split (give = owned items, get = every item, with the ones your
 * stock can't cover flagged unaffordable), the "never both + disabled" guarantee that
 * flagging buys us, the Healing tab's Potion <-> Fresh Water default, and that checkout()
 * writes exactly the preview -- across bag + PC storage, money only ever going up.
 *
 * Built on a real Bridge over BaseSAV; each test sets the item counts it needs, so the
 * tests are order-independent.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>

#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/itemexchangemodel.h>

using namespace pse_test;

class TestItemExchange : public QObject
{
  Q_OBJECT

private:
  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;
  ItemExchangeModel* m_exm = nullptr;
  ItemStorageBox* m_bag = nullptr;
  ItemStorageBox* m_storage = nullptr;

  /// Item index for an internal name ("POTION"), or -1.
  int indOf(const QString& name) const
  {
    auto* e = ItemsDB::inst()->getIndAt(name);
    return (e == nullptr) ? -1 : e->getInd();
  }

  int buyOf(const QString& name) const
  {
    auto* e = ItemsDB::inst()->getIndAt(name);
    return (e == nullptr) ? 0 : e->buyPriceMoney();
  }

  int owned(int ind) const { return m_bag->amountOfInd(ind) + m_storage->amountOfInd(ind); }

  /// Force the player to hold exactly @p n of @p ind (in the bag), wiping any prior stock
  /// so each test starts from a state it fully controls.
  void setOwned(int ind, int n)
  {
    m_bag->removeAmount(ind, 9999);
    m_storage->removeAmount(ind, 9999);
    if(n > 0)
      m_bag->addAmount(ind, n);
    QCOMPARE(owned(ind), n);
  }

  void setMoney(int m)
  {
    m_brg->file->data->dataExpanded->player->basics->money = static_cast<unsigned int>(m);
  }

  int money() const
  {
    return static_cast<int>(m_brg->file->data->dataExpanded->player->basics->money);
  }

  /// Select the give/get pair by internal name and clear any queued steps.
  void selectPair(const QString& give, const QString& get)
  {
    m_exm->setItemAInd(indOf(give));
    m_exm->setItemBInd(indOf(get));
    m_exm->reset();
  }

  static int ceilDiv(int a, int b) { return (b <= 0) ? 0 : ((a + b - 1) / b); }

  /// Is @p ind present in a sourceItems()/targetItems() list?
  static bool listHas(const QVariantList& list, int ind)
  {
    for(const QVariant& v : list) {
      if(v.toMap().value(QStringLiteral("ind")).toInt() == ind)
        return true;
    }
    return false;
  }

  /// The `affordable` flag targetItems() attached to @p ind (false if absent).
  static bool listAffordable(const QVariantList& list, int ind)
  {
    for(const QVariant& v : list) {
      const QVariantMap m = v.toMap();
      if(m.value(QStringLiteral("ind")).toInt() == ind)
        return m.value(QStringLiteral("affordable")).toBool();
    }
    return false;
  }

private slots:
  void initTestCase();
  void cleanupTestCase();

  void priceData_isWhatTheseTestsAssume();

  void evenTrade_costsExactWholeItems_andRefundsNothing();
  void unevenTrade_roundsUpTheTotalOnce_notEachStep();
  void refund_isNeverNegative_andMoneyOnlyGoesUp();

  void checkout_writesExactlyThePreview();
  void checkout_spillsAcrossBagAndStorage();

  void giveList_isOwnedOnly_getList_isEveryItem();
  void getList_flagsWhatTheStockCannotCover();
  void plusButtons_areNeverBothDisabled();

  void healingDefault_isPotionToFreshWater();
  void healingDefault_prefersPotionFamilyOverStatusHeals();

  void gating_stopsWhenTheSourceRunsOut();
};

void TestItemExchange::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();
  m_file = new FileManagement;
  loadInto(*m_file->data, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
  m_brg = new Bridge(m_file);

  m_exm     = m_brg->itemExchangeModel;
  m_bag     = m_file->data->dataExpanded->player->items;
  m_storage = m_file->data->dataExpanded->storage->items;

  QVERIFY(m_exm != nullptr);
  QVERIFY(m_bag != nullptr);
  QVERIFY(m_storage != nullptr);
}

void TestItemExchange::cleanupTestCase()
{
  delete m_brg;  m_brg = nullptr;
  delete m_file; m_file = nullptr;
  m_exm = nullptr; m_bag = nullptr; m_storage = nullptr;
}

// Guard rail: the worked examples below use the real Gen 1 buy prices. If items.json ever
// changed one of them, fail HERE (with a clear reason) rather than in a confusing way in
// the arithmetic tests underneath.
void TestItemExchange::priceData_isWhatTheseTestsAssume()
{
  QCOMPARE(buyOf(QStringLiteral("POTION")), 300);
  QCOMPARE(buyOf(QStringLiteral("FRESH WATER")), 200);
  QCOMPARE(buyOf(QStringLiteral("ANTIDOTE")), 100);
  QCOMPARE(buyOf(QStringLiteral("AWAKENING")), 200);
  QCOMPARE(buyOf(QStringLiteral("FULL RESTORE")), 3000);
}

// THE headline rule (Twilight's own example): 3 Fresh Water is ₽600, which is exactly
// 2 Potions -- so 2 Potions are spent and NO money is refunded. Per-step pricing would
// have wrongly taken 3 Potions and refunded ₽300.
void TestItemExchange::evenTrade_costsExactWholeItems_andRefundsNothing()
{
  const int potion = indOf(QStringLiteral("POTION"));
  const int water  = indOf(QStringLiteral("FRESH WATER"));

  setOwned(potion, 10);
  setOwned(water, 0);
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  m_exm->adjust(-1);
  m_exm->adjust(-1);
  m_exm->adjust(-1);            // 3 Fresh Water

  QCOMPARE(m_exm->net(), -3);
  QCOMPARE(m_exm->giveFor(-1, 3), 2);          // exactly two whole Potions
  QCOMPARE(m_exm->refundFor(-1, 3), 0);        // and nothing left over
  QCOMPARE(m_exm->aAfter(), 8);                // 10 - 2
  QCOMPARE(m_exm->bAfter(), 3);
  QCOMPARE(m_exm->moneyAfter(), m_exm->moneyStart());
}

// The total (not each step) is what gets rounded up, so the leftover is charged ONCE.
// 4 Fresh Water = ₽800 -> 3 Potions (₽900), refund ₽100. Per-step would be 4 Potions and
// a ₽400 refund.
void TestItemExchange::unevenTrade_roundsUpTheTotalOnce_notEachStep()
{
  setOwned(indOf(QStringLiteral("POTION")), 10);
  setOwned(indOf(QStringLiteral("FRESH WATER")), 0);
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  QCOMPARE(m_exm->giveFor(-1, 4), 3);
  QCOMPARE(m_exm->refundFor(-1, 4), 100);

  // And it must never be a per-step multiple: one step costs 1 Potion / ₽100 back, but
  // four steps cost far less than 4x that.
  QCOMPARE(m_exm->giveFor(-1, 1), 1);
  QCOMPARE(m_exm->refundFor(-1, 1), 100);
  QVERIFY2(m_exm->giveFor(-1, 4) < 4 * m_exm->giveFor(-1, 1),
           "four steps were priced as four separate trades");
  QVERIFY2(m_exm->refundFor(-1, 4) < 4 * m_exm->refundFor(-1, 1),
           "four steps refunded four separate leftovers");
}

// Whatever the pair, the refund is the single rounding leftover: 0 <= refund < the given
// item's price (a full unit of change would have meant we took one item too many), and the
// previewed money never drops below where it started.
void TestItemExchange::refund_isNeverNegative_andMoneyOnlyGoesUp()
{
  setOwned(indOf(QStringLiteral("POTION")), 50);
  setOwned(indOf(QStringLiteral("FRESH WATER")), 50);
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  const int aBuy = m_exm->aBuy();
  const int bBuy = m_exm->bBuy();

  for(int n = 1; n <= 12; n++) {
    // Getting the B item, paying in A.
    const int giveB = m_exm->giveFor(-1, n);
    const int refB  = m_exm->refundFor(-1, n);
    QCOMPARE(giveB, ceilDiv(n * bBuy, aBuy));
    QCOMPARE(refB, giveB * aBuy - n * bBuy);
    QVERIFY2(refB >= 0, "refund went negative");
    QVERIFY2(refB < aBuy, "refund was a whole extra item of change");

    // Mirror: getting the A item, paying in B.
    const int giveA = m_exm->giveFor(1, n);
    const int refA  = m_exm->refundFor(1, n);
    QCOMPARE(giveA, ceilDiv(n * aBuy, bBuy));
    QCOMPARE(refA, giveA * bBuy - n * aBuy);
    QVERIFY2(refA >= 0, "refund went negative");
    QVERIFY2(refA < bBuy, "refund was a whole extra item of change");
  }

  // Nothing queued must cost or refund anything.
  QCOMPARE(m_exm->giveFor(-1, 0), 0);
  QCOMPARE(m_exm->refundFor(-1, 0), 0);
  QCOMPARE(m_exm->moneyAfter(), m_exm->moneyStart());
}

// The preview is a promise: checkout() must write exactly the counts and money it showed.
void TestItemExchange::checkout_writesExactlyThePreview()
{
  const int potion = indOf(QStringLiteral("POTION"));
  const int water  = indOf(QStringLiteral("FRESH WATER"));

  setOwned(potion, 10);
  setOwned(water, 0);
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  m_exm->adjust(-1);
  m_exm->adjust(-1);
  m_exm->adjust(-1);
  m_exm->adjust(-1);   // 4 Fresh Water: 3 Potions, ₽100 back

  const int expectedA     = m_exm->aAfter();
  const int expectedB     = m_exm->bAfter();
  const int expectedMoney = m_exm->moneyAfter();

  m_exm->checkout();

  QCOMPARE(owned(potion), expectedA);
  QCOMPARE(owned(water), expectedB);
  QCOMPARE(money(), expectedMoney);

  QCOMPARE(owned(potion), 7);
  QCOMPARE(owned(water), 4);
  QCOMPARE(money(), 100100);

  QCOMPARE(m_exm->net(), 0);   // the queue is consumed
}

// Items are counted across the bag AND the PC storage, and consumed bag-first -- so a
// trade can legally draw on a source the bag alone couldn't cover.
void TestItemExchange::checkout_spillsAcrossBagAndStorage()
{
  const int potion = indOf(QStringLiteral("POTION"));
  const int water  = indOf(QStringLiteral("FRESH WATER"));

  setOwned(potion, 0);
  setOwned(water, 0);
  m_bag->addAmount(potion, 2);       // 2 in the bag ...
  m_storage->addAmount(potion, 3);   // ... 3 in the PC
  QCOMPARE(owned(potion), 5);
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  // 6 Fresh Water = ₽1200 = exactly 4 Potions, no refund -- more Potions than the bag holds.
  for(int i = 0; i < 6; i++)
    m_exm->adjust(-1);

  QCOMPARE(m_exm->net(), -6);
  QCOMPARE(m_exm->giveFor(-1, 6), 4);
  QCOMPARE(m_exm->refundFor(-1, 6), 0);

  m_exm->checkout();

  QCOMPARE(owned(potion), 1);                    // 5 - 4, drawn bag-first then PC
  QCOMPARE(m_bag->amountOfInd(potion), 0);       // bag drained first
  QCOMPARE(m_storage->amountOfInd(potion), 1);
  QCOMPARE(owned(water), 6);
  QCOMPARE(money(), 100000);                     // even trade: no refund
}

// The give side offers only what you HAVE; the get side offers EVERYTHING -- including
// items you own none of (that's the whole point of the change).
void TestItemExchange::giveList_isOwnedOnly_getList_isEveryItem()
{
  const int potion = indOf(QStringLiteral("POTION"));
  const int water  = indOf(QStringLiteral("FRESH WATER"));

  setOwned(potion, 5);
  setOwned(water, 0);       // owned NONE of this one
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  const QVariantList give = m_exm->sourceItems(false, -1);
  QVERIFY2(listHas(give, potion), "an owned item was missing from the give list");
  QVERIFY2(!listHas(give, water), "an unowned item leaked into the give list");

  const QVariantList get = m_exm->targetItems(false, potion);
  QVERIFY2(listHas(get, water), "an unowned item was missing from the get list");
  QVERIFY2(get.size() > give.size(), "the get list should be broader than the give list");

  // Neither list may offer the item the other side already has (no trading X for X).
  QVERIFY2(!listHas(m_exm->targetItems(false, potion), potion), "get list offered the give item");
  QVERIFY2(!listHas(m_exm->sourceItems(false, water), water), "give list offered the get item");
}

// Listing every item would be a trap if you could pick one you can't pay for -- so the ones
// your stock can't cover come back flagged, and the dropdown greys them.
void TestItemExchange::getList_flagsWhatTheStockCannotCover()
{
  const int potion  = indOf(QStringLiteral("POTION"));
  const int water   = indOf(QStringLiteral("FRESH WATER"));
  const int restore = indOf(QStringLiteral("FULL RESTORE"));

  setOwned(potion, 1);          // ₽300 of buying power ...
  setOwned(water, 0);
  setOwned(restore, 0);
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  const QVariantList get = m_exm->targetItems(false, potion);

  // ... covers a ₽200 Fresh Water ...
  QVERIFY2(listAffordable(get, water), "Fresh Water should be affordable for one Potion");
  QVERIFY(m_exm->canGainTarget(water));

  // ... but not a ₽3000 Full Restore (which needs 10 Potions). Still LISTED, just flagged.
  QVERIFY2(listHas(get, restore), "Full Restore should still be listed");
  QVERIFY2(!listAffordable(get, restore), "Full Restore is not payable with one Potion");
  QVERIFY(!m_exm->canGainTarget(restore));

  // Give it enough Potions and the same item becomes reachable.
  setOwned(potion, 10);
  QVERIFY2(m_exm->canGainTarget(restore), "10 Potions (₽3000) should cover a Full Restore");
  QVERIFY2(listAffordable(m_exm->targetItems(false, potion), restore),
           "the flag must track the live stock");
}

// The guarantee that flagging buys us: on any pair you can actually select, at least one
// "+" is live. A dead card with both buttons greyed must not be reachable.
void TestItemExchange::plusButtons_areNeverBothDisabled()
{
  const int potion = indOf(QStringLiteral("POTION"));

  setOwned(potion, 4);
  setMoney(100000);

  // Every affordable target, against a source we own: the get "+" must work.
  const QVariantList get = m_exm->targetItems(false, potion);
  int checked = 0;
  for(const QVariant& v : get) {
    const QVariantMap m = v.toMap();
    if(!m.value(QStringLiteral("affordable")).toBool())
      continue;   // greyed in the UI -- not selectable, so not a "both disabled" case

    m_exm->setItemAInd(potion);
    m_exm->setItemBInd(m.value(QStringLiteral("ind")).toInt());
    m_exm->reset();

    QVERIFY2(m_exm->canAddA() || m_exm->canAddB(),
             qPrintable(QStringLiteral("both + buttons disabled on a selectable pair: %1")
                          .arg(m.value(QStringLiteral("name")).toString())));
    QVERIFY2(m_exm->canAddB(), "the get side must be live for an affordable target");
    checked++;
  }
  QVERIFY2(checked > 0, "no affordable targets to check -- the fixture is wrong");

  // And once a step is queued, the opposite side comes back, so you can always undo.
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));
  m_exm->adjust(-1);
  QVERIFY2(m_exm->canAddA(), "after gaining one, the reverse + must be available to undo");
}

// The Healing tab opens ready to use: Potion (given) <-> Fresh Water (got).
void TestItemExchange::healingDefault_isPotionToFreshWater()
{
  setOwned(indOf(QStringLiteral("POTION")), 5);
  setOwned(indOf(QStringLiteral("FRESH WATER")), 0);
  setMoney(100000);

  m_exm->pickDefaults(true);

  QCOMPARE(m_exm->itemAInd(), indOf(QStringLiteral("POTION")));
  QCOMPARE(m_exm->itemBInd(), indOf(QStringLiteral("FRESH WATER")));
  QCOMPARE(m_exm->net(), 0);
  QVERIFY2(m_exm->canAddB(), "the default pair must be immediately tradeable");
}

// With no plain Potion on hand it falls to the next POTION-FAMILY item -- not to whatever
// healing item happens to sort first (Antidote), which is what a naive default would do.
void TestItemExchange::healingDefault_prefersPotionFamilyOverStatusHeals()
{
  setOwned(indOf(QStringLiteral("POTION")), 0);        // no plain Potions ...
  setOwned(indOf(QStringLiteral("SUPER POTION")), 8);  // ... but Super Potions ...
  setOwned(indOf(QStringLiteral("ANTIDOTE")), 25);     // ... and plenty of Antidotes
  setOwned(indOf(QStringLiteral("FRESH WATER")), 0);
  setMoney(100000);

  m_exm->pickDefaults(true);

  QCOMPARE(m_exm->itemAInd(), indOf(QStringLiteral("SUPER POTION")));
  QVERIFY2(m_exm->itemAInd() != indOf(QStringLiteral("ANTIDOTE")),
           "a status heal was chosen over a potion the player actually holds");
  QCOMPARE(m_exm->itemBInd(), indOf(QStringLiteral("FRESH WATER")));
  QVERIFY(m_exm->canAddB());
}

// You cannot queue a trade the save can't pay for: the "+" shuts off exactly when the
// source can no longer cover one more, and never lets a count go negative.
void TestItemExchange::gating_stopsWhenTheSourceRunsOut()
{
  const int potion = indOf(QStringLiteral("POTION"));
  const int water  = indOf(QStringLiteral("FRESH WATER"));

  setOwned(potion, 2);     // ₽600 of buying power = 3 Fresh Water, exactly
  setOwned(water, 0);
  setMoney(100000);
  selectPair(QStringLiteral("POTION"), QStringLiteral("FRESH WATER"));

  int steps = 0;
  while(m_exm->canAddB() && steps < 50) {
    m_exm->adjust(-1);
    steps++;
    QVERIFY2(m_exm->aAfter() >= 0, "the source count went negative");
    QVERIFY2(m_exm->moneyAfter() >= m_exm->moneyStart(), "money went DOWN on an exchange");
  }

  QCOMPARE(steps, 3);                        // ₽600 buys three ₽200 Fresh Waters, no more
  QVERIFY2(!m_exm->canAddB(), "the + stayed live past what the stock could pay for");
  QCOMPARE(m_exm->giveFor(-1, 3), 2);
  QCOMPARE(m_exm->refundFor(-1, 3), 0);
  QCOMPARE(m_exm->aAfter(), 0);              // spent to the last Potion, exactly

  // Pushing again must be a no-op, not an over-spend.
  m_exm->adjust(-1);
  QCOMPARE(m_exm->net(), -3);
}

QTEST_MAIN(TestItemExchange)
#include "tst_item_exchange.moc"
