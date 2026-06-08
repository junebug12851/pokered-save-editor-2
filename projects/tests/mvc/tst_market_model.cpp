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

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void allFourModes_buildWithoutCrash();
  void sellMode_hasItemsAndMoneyStart();
  void cart_setCountUpdatesTotals();
  void sellCheckout_raisesMoney();
  void buyCheckout_lowersMoney();
};

void TestMarketModel::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
  Router::loadScreens();
}

void TestMarketModel::init()
{
  m_file = new FileManagement;
  loadInto(*m_file->data, readSaveBytes(QStringLiteral("BaseSAV.sav")));
  m_brg = new Bridge(m_file);
  m_mkt = m_brg->marketModel;
}

void TestMarketModel::cleanup()
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
  const int row = findCartableRow(/*needWorth*/true);
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
  // Find a store row we can both cart and afford one of.
  int row = -1;
  for(int i = 0; i < m_mkt->rowCount(QModelIndex()); i++) {
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

QTEST_GUILESS_MAIN(TestMarketModel)
#include "tst_market_model.moc"
