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
 * @file tst_market_model.cpp
 * @brief The Poke-mart ItemMarketModel: builds its row list for each of the four
 *        modes (buy/sell x money/coins), tracks a cart via setData(CartCountRole),
 *        reports totals, and applies a transaction in checkout(). Verified end to
 *        end with a sell that actually raises the player's money. Built on a real
 *        Bridge over BaseSAV; fresh fixture per test.
 */

#include <QtTest>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <mvc/itemmarketmodel.h>
#include <mvc/itemmarketcartmodel.h>

using namespace pse_test;

class TestMarketModel : public QObject
{
  Q_OBJECT

private:
  FileManagement* m_file = nullptr;
  Bridge* m_brg = nullptr;
  ItemMarketModel* m_mkt = nullptr;

  void setMode(bool buy, bool money)
  {
    m_mkt->isExchangeMode = false; // defensive: leave exchange mode if a prior test set it
    m_mkt->isBuyMode = buy;
    m_mkt->isMoneyCurrency = money;
    m_mkt->reUpdateAll(); // member writes don't fire NOTIFY, so rebuild explicitly
  }

  int roleInt(int row, int role) { return m_mkt->data(m_mkt->index(row), role).toInt(); }

  // First row that currently has cart capacity (and, if requested, positive worth).
  int findCartableRow(bool needWorth)
  {
    for(int i = 0; i < m_mkt->rowCount(QModelIndex()); i++) {
      if(roleInt(i, ItemMarketModel::OnCartLeftRole) > 0 &&
         (!needWorth || roleInt(i, ItemMarketModel::ItemWorthRole) > 0))
        return i;
    }
    return -1;
  }

  // The unified list now holds BOTH buy and sell rows, so direction-sensitive
  // tests must pick a specific type ("playerItem" = sell, "storeItem" = buy).
  QString typeOf(int row)
  { return m_mkt->data(m_mkt->index(row), ItemMarketModel::WhichTypeRole).toString(); }

  int findRowOfType(const QString& type, bool needWorth)
  {
    for(int i = 0; i < m_mkt->rowCount(QModelIndex()); i++) {
      if(typeOf(i) == type &&
         roleInt(i, ItemMarketModel::OnCartLeftRole) > 0 &&
         (!needWorth || roleInt(i, ItemMarketModel::ItemWorthRole) > 0))
        return i;
    }
    return -1;
  }

private slots:
  void initTestCase();
  void cleanupTestCase();

  void allFourModes_buildWithoutCrash();
  void sellMode_hasItemsAndMoneyStart();
  void cart_setCountUpdatesTotals();
  void sellCheckout_raisesMoney();
  void buyCheckout_lowersMoney();
  void coinsMode_rowsAndRolesExercised();
  void cartModel_filtersToCartedRows();
  void exchangeMode_swapsMoneyForCoins();
  void exchangeAdjust_netAxis();
  void unifiedCart_buyAndSellNetTogether();
};

// One Bridge for the whole case (built once, like the real app -- which never
// creates/destroys Bridges repeatedly). Per-test new/delete of a Bridge churns its
// many signal connections + the static ItemMarketEntry pointers and intermittently
// crashed via a queued slot firing on a torn-down object. All assertions below are
// relative (money up/down vs. each test's own baseline), so a shared fixture is
// order-independent.
void TestMarketModel::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();
  m_file = new FileManagement;
  loadInto(*m_file->data, readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")));
  m_brg = new Bridge(m_file);
  m_mkt = m_brg->marketModel;
}

void TestMarketModel::cleanupTestCase()
{
  delete m_brg; m_brg = nullptr;
  delete m_file; m_file = nullptr;
  m_mkt = nullptr;
}

void TestMarketModel::allFourModes_buildWithoutCrash()
{
  for(int buy = 0; buy <= 1; buy++)
    for(int money = 0; money <= 1; money++) {
      setMode(buy, money);
      QVERIFY2(m_mkt->rowCount(QModelIndex()) >= 0, "negative market row count");
      (void)m_mkt->whichMode(); // combined-mode getter must not crash
    }
}

void TestMarketModel::sellMode_hasItemsAndMoneyStart()
{
  setMode(/*buy*/false, /*money*/true);
  QVERIFY2(m_mkt->rowCount(QModelIndex()) > 0, "sell list empty (player has items)");
  QCOMPARE(m_mkt->totalCartCount(), 0);                  // nothing carted yet
  QCOMPARE(m_mkt->moneyStart(), int(m_file->data->dataExpanded->player->basics->money));
}

void TestMarketModel::cart_setCountUpdatesTotals()
{
  setMode(false, true);
  const int row = findCartableRow(/*needWorth*/false);
  if(row < 0) QSKIP("no cartable row in sell/money mode");

  QVERIFY(m_mkt->setData(m_mkt->index(row), QVariant(1), ItemMarketModel::CartCountRole));
  QVERIFY2(m_mkt->totalCartCount() >= 1, "cart count did not rise after setData");

  m_mkt->setData(m_mkt->index(row), QVariant(0), ItemMarketModel::CartCountRole);
  QCOMPARE(m_mkt->totalCartCount(), 0);                  // clearing the cart resets the total
}

void TestMarketModel::sellCheckout_raisesMoney()
{
  setMode(false, true);
  const int row = findRowOfType("playerItem", /*needWorth*/true);
  if(row < 0) QSKIP("no positively-valued sellable row");

  const unsigned int before = m_file->data->dataExpanded->player->basics->money;
  QVERIFY(m_mkt->setData(m_mkt->index(row), QVariant(1), ItemMarketModel::CartCountRole));
  QVERIFY(m_mkt->canAnyCheckout());
  m_mkt->checkout();

  QVERIFY2(m_file->data->dataExpanded->player->basics->money > before,
           "selling an item did not raise the player's money");
}

void TestMarketModel::buyCheckout_lowersMoney()
{
  setMode(/*buy*/true, /*money*/true);
  if(m_mkt->rowCount(QModelIndex()) == 0) QSKIP("buy/money store list is empty");

  const int money = m_mkt->moneyStart();
  // Find a STORE (buy) row we can both cart and afford one of.
  int row = -1;
  for(int i = 0; i < m_mkt->rowCount(QModelIndex()); i++) {
    if(typeOf(i) != "storeItem") continue;
    const int worth = roleInt(i, ItemMarketModel::ItemWorthRole);
    if(roleInt(i, ItemMarketModel::OnCartLeftRole) > 0 && worth > 0 && worth <= money) { row = i; break; }
  }
  if(row < 0) QSKIP("no affordable, cartable store item");

  const unsigned int before = m_file->data->dataExpanded->player->basics->money;
  QVERIFY(m_mkt->setData(m_mkt->index(row), QVariant(1), ItemMarketModel::CartCountRole));
  QVERIFY(m_mkt->canAnyCheckout());
  m_mkt->checkout();

  QVERIFY2(m_file->data->dataExpanded->player->basics->money < before,
           "buying an item did not lower the player's money");
}

void TestMarketModel::coinsMode_rowsAndRolesExercised()
{
  // Coins currency = the Game Corner. buildList prepends a Money-Exchange message +
  // money entry, and (buy) lists GC-pokemon entries. Sweep data() across every role
  // for every row in both buy and sell coins modes to drive each entry subtype's
  // virtual accessors (_name/_inStockCount/_itemWorth/onCartLeft/...).
  const QHash<int, QByteArray> roles = m_mkt->roleNames();
  QVERIFY(!roles.isEmpty());

  for(int buy = 0; buy <= 1; buy++) {
    setMode(buy == 1, /*money*/false);
    const int n = m_mkt->rowCount(QModelIndex());
    QVERIFY2(n > 0, "coins-mode list is empty (expected at least the money-exchange rows)");
    for(int r = 0; r < n; r++) {
      const QModelIndex idx = m_mkt->index(r);
      for(auto it = roles.constBegin(); it != roles.constEnd(); ++it)
        m_mkt->data(idx, it.key()); // drive every entry accessor; values vary by subtype
    }
  }

  // Cart a row in coins mode if one is available, to drive the cart-count path too.
  // (GC entries handle the cart differently from plain items, so we just drive the
  // path rather than assert a specific total.)
  setMode(/*buy*/true, /*money*/false);
  const int row = findCartableRow(/*needWorth*/false);
  if(row >= 0)
    m_mkt->setData(m_mkt->index(row), QVariant(1), ItemMarketModel::CartCountRole);
}

void TestMarketModel::cartModel_filtersToCartedRows()
{
  // brg.marketCartModel is a cart-only filter proxy over the market model -- it
  // backs the Pokemart receipt pane. It should show exactly the rows currently on
  // the cart (cartCount > 0), never a section-header ("msg") row, and update live as
  // the cart changes.
  setMode(/*buy*/false, /*money*/true);   // rebuild clears the cart
  ItemMarketCartModel* cart = m_brg->marketCartModel;
  QVERIFY(cart != nullptr);
  QCOMPARE(cart->rowCount(QModelIndex()), 0);            // empty cart -> empty receipt

  const int row = findCartableRow(/*needWorth*/false);
  if(row < 0) QSKIP("no cartable row in sell/money mode");
  const QString name =
      m_mkt->data(m_mkt->index(row), ItemMarketModel::NameRole).toString();

  QVERIFY(m_mkt->setData(m_mkt->index(row), QVariant(2), ItemMarketModel::CartCountRole));
  QCOMPARE(cart->rowCount(QModelIndex()), 1);            // exactly the carted row
  const QModelIndex idx = cart->index(0, 0);
  QCOMPARE(cart->data(idx, ItemMarketModel::NameRole).toString(), name);
  QCOMPARE(cart->data(idx, ItemMarketModel::CartCountRole).toInt(), 2);
  QVERIFY2(cart->data(idx, ItemMarketModel::WhichTypeRole).toString() != "msg",
           "a section-header row leaked into the receipt");

  m_mkt->setData(m_mkt->index(row), QVariant(0), ItemMarketModel::CartCountRole);
  QCOMPARE(cart->rowCount(QModelIndex()), 0);            // clearing removes it live
}

void TestMarketModel::exchangeMode_swapsMoneyForCoins()
{
  // Exchange mode is its own list: a header + the two money<->coins swap rows
  // (Money=>Coins, then Coins=>Money), pulled out of the buy/sell item lists.
  m_mkt->isExchangeMode = true;
  m_mkt->reUpdateAll();

  QCOMPARE(m_mkt->rowCount(QModelIndex()), 3);
  const int row = 1; // the Money=>Coins (buying) direction
  QCOMPARE(m_mkt->data(m_mkt->index(row), ItemMarketModel::WhichTypeRole).toString(),
           QString("money"));

  const int moneyStart = m_mkt->exchangeMoneyStart();
  const int coinsStart = m_mkt->exchangeCoinsStart();
  const int buyRate    = m_mkt->exchangeBuyRate();   // money cost of ONE coin (~20)
  QVERIFY2(buyRate > 0, "game corner buy rate should be positive");
  if(moneyStart < buyRate) {
    m_mkt->isExchangeMode = false; m_mkt->reUpdateAll();
    QSKIP("player can't afford a single coin");
  }

  // onCart = number of COINS. Buying 1 coin costs `buyRate` money and gains 1 coin.
  QVERIFY(m_mkt->setData(m_mkt->index(row), QVariant(1), ItemMarketModel::CartCountRole));
  QCOMPARE(m_mkt->exchangeMoneyAfter(), moneyStart - buyRate);
  QCOMPARE(m_mkt->exchangeCoinsAfter(), coinsStart + 1);

  QVERIFY(m_mkt->canAnyCheckout());
  m_mkt->checkout();
  QCOMPARE((int)m_file->data->dataExpanded->player->basics->money, moneyStart - buyRate);
  QCOMPARE((int)m_file->data->dataExpanded->player->basics->coins, coinsStart + 1);

  m_mkt->isExchangeMode = false; // leave the shared fixture in buy/sell mode
  m_mkt->reUpdateAll();
}

void TestMarketModel::exchangeAdjust_netAxis()
{
  // The converter drives ONE net coin axis: +Coins buys (net+1), +Money sells (net-1).
  m_mkt->isExchangeMode = true;
  m_mkt->reUpdateAll();

  const int moneyStart = m_mkt->exchangeMoneyStart();
  const int coinsStart = m_mkt->exchangeCoinsStart();
  const int buyRate    = m_mkt->exchangeBuyRate();
  const int sellRate   = m_mkt->exchangeSellRate();
  QCOMPARE(m_mkt->exchangeNet(), 0);

  if(moneyStart >= buyRate) {
    m_mkt->exchangeAdjust(+1);                       // buy 1 coin
    QCOMPARE(m_mkt->exchangeNet(), 1);
    QCOMPARE(m_mkt->exchangeMoneyAfter(), moneyStart - buyRate);
    QCOMPARE(m_mkt->exchangeCoinsAfter(), coinsStart + 1);
    m_mkt->exchangeAdjust(-1);                        // +Money cancels it back to 0
    QCOMPARE(m_mkt->exchangeNet(), 0);
    QCOMPARE(m_mkt->exchangeMoneyAfter(), moneyStart);
  }

  if(coinsStart >= 1) {
    m_mkt->exchangeAdjust(-1);                        // sell 1 coin
    QCOMPARE(m_mkt->exchangeNet(), -1);
    QCOMPARE(m_mkt->exchangeMoneyAfter(), moneyStart + sellRate);
    QCOMPARE(m_mkt->exchangeCoinsAfter(), coinsStart - 1);
  }

  m_mkt->exchangeAdjust(-1000000);                    // clamps, never goes below owned coins
  QVERIFY2(m_mkt->exchangeCoinsAfter() >= 0, "sold more coins than owned");

  m_mkt->isExchangeMode = false;
  m_mkt->reUpdateAll();
}

void TestMarketModel::unifiedCart_buyAndSellNetTogether()
{
  // One single-currency cart now holds BOTH a buy and a sell at once; the total is
  // the signed net (sell + , buy -), and the balance-after follows it.
  setMode(/*buy*/false, /*money*/true); // Pokemart (money) cart; builds buy + sell
  const int sellRow = findRowOfType("playerItem", /*needWorth*/true);
  const int buyRow  = findRowOfType("storeItem",  /*needWorth*/true);
  if(sellRow < 0 || buyRow < 0)
    QSKIP("need both a sellable item and a positively-priced store item");

  const int sellWorth = roleInt(sellRow, ItemMarketModel::ItemWorthRole); // qty 1
  const int buyWorth  = roleInt(buyRow,  ItemMarketModel::ItemWorthRole);
  const int start     = m_mkt->moneyStart();

  QVERIFY(m_mkt->setData(m_mkt->index(sellRow), QVariant(1), ItemMarketModel::CartCountRole));
  QVERIFY(m_mkt->setData(m_mkt->index(buyRow),  QVariant(1), ItemMarketModel::CartCountRole));

  // Both live in the cart together.
  QVERIFY2(m_mkt->totalCartCount() >= 2, "buy and sell did not both stay on the cart");

  // Signed net: sell adds, buy subtracts. Balance after = start + net.
  QCOMPARE(m_mkt->totalCartWorth(), sellWorth - buyWorth);
  QCOMPARE(m_mkt->moneyLeftover(), start + (sellWorth - buyWorth));

  // Per-row sign roles back the receipt's +/-.
  QCOMPARE(m_mkt->data(m_mkt->index(sellRow), ItemMarketModel::CartSignRole).toInt(), 1);
  QCOMPARE(m_mkt->data(m_mkt->index(buyRow),  ItemMarketModel::CartSignRole).toInt(), -1);

  m_mkt->setData(m_mkt->index(sellRow), QVariant(0), ItemMarketModel::CartCountRole);
  m_mkt->setData(m_mkt->index(buyRow),  QVariant(0), ItemMarketModel::CartCountRole);
  QCOMPARE(m_mkt->totalCartCount(), 0);
}

QTEST_GUILESS_MAIN(TestMarketModel)
#include "tst_market_model.moc"
