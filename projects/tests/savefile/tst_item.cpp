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
 * @file tst_item.cpp
 * @brief Unit coverage for the Item inventory-slot fragment: its constructors
 *        (null-iterator, index+amount, name+amount, random), the load() overloads,
 *        randomize(), toItem() resolution, the full buy/sell pricing surface
 *        (one + all, money + coins, and the null-item -> 0 guards), canSell(), and
 *        setAmount() clamping. Pricing/identity are validated against the ItemsDB
 *        entry the slot resolves to (independent oracle).
 */

#include <QtTest>

#include <pse-db/db.h>
#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>
#include <pse-savefile/expanded/fragments/item.h>

class TestItem : public QObject
{
  Q_OBJECT

private:
  ItemDBEntry* m_good = nullptr; // a non-glitch, non-once, sellable item
  int m_goodInd = -1;
  QString m_goodName;
  int m_badInd = -1;             // an index that resolves to no item (if any)

private slots:
  void initTestCase();
  void ctors_coverEveryForm();
  void load_overloads_includingInvalidName();
  void randomize_picksLegalItem();
  void toItem_andPricing_matchDbEntry();
  void invalidItem_pricingIsZeroAndCannotSell();
  void setAmount_clampsToOneThrough99();
};

void TestItem::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);

  for(auto* e : ItemsDB::inst()->getStore()) {
    if(e != nullptr && !e->getGlitch() && !e->getOnce() && e->canSell()) {
      m_good = e; m_goodInd = e->getInd(); m_goodName = e->getName();
      break;
    }
  }
  QVERIFY2(m_good != nullptr, "no non-glitch sellable item found in ItemsDB");
  QVERIFY(!m_goodName.isEmpty());

  // An index that maps to no item -- for the null-resolution guards.
  for(int i = 255; i >= 0; --i) {
    if(ItemsDB::inst()->getIndAt(QString::number(i)) == nullptr) { m_badInd = i; break; }
  }
}

void TestItem::ctors_coverEveryForm()
{
  // Null iterator -> reset.
  Item a(static_cast<SaveFileIterator*>(nullptr));
  QCOMPARE(a.ind, 0);
  QCOMPARE(a.amount, 0);

  // Index + amount.
  Item b(var8(m_goodInd), var8(5));
  QCOMPARE(b.ind, m_goodInd);
  QCOMPARE(b.amount, 5);

  // Name + amount.
  Item c(m_goodName, 7);
  QCOMPARE(c.ind, m_goodInd);
  QCOMPARE(c.amount, 7);

  // random=false -> reset; random=true -> a legal item.
  Item d(false);
  QCOMPARE(d.ind, 0);
  Item e(true);
  QVERIFY(e.toItem() != nullptr);
}

void TestItem::load_overloads_includingInvalidName()
{
  Item i(static_cast<SaveFileIterator*>(nullptr));

  i.load(9, 4);
  QCOMPARE(i.ind, 9);
  QCOMPARE(i.amount, 4);

  i.load(m_goodName, 2);
  QCOMPARE(i.ind, m_goodInd);
  QCOMPARE(i.amount, 2);

  // Invalid name leaves ind untouched (the tmp != nullptr guard) but sets amount.
  const int indBefore = i.ind;
  i.load(QStringLiteral("__definitely_not_an_item__"), 6);
  QCOMPARE(i.ind, indBefore);
  QCOMPARE(i.amount, 6);

  // load(bool): false resets, true randomizes to a legal item.
  i.load(false);
  QCOMPARE(i.ind, 0);
  i.load(true);
  QVERIFY(i.toItem() != nullptr);
}

void TestItem::randomize_picksLegalItem()
{
  Item i(static_cast<SaveFileIterator*>(nullptr));
  for(int n = 0; n < 50; n++) {
    i.randomize();
    ItemDBEntry* e = i.toItem();
    QVERIFY2(e != nullptr, "randomized to an unresolvable item");
    QVERIFY2(!e->getGlitch(), "randomized to a glitch item");
    QVERIFY2(!e->getOnce(), "randomized to a one-time item");
    QVERIFY(i.amount >= 1 && i.amount <= 5);
  }
}

void TestItem::toItem_andPricing_matchDbEntry()
{
  Item i(var8(m_goodInd), var8(3));

  ItemDBEntry* el = i.toItem();
  QVERIFY(el != nullptr);
  QCOMPARE(el->getInd(), m_goodInd);

  QCOMPARE(i.canSell(), el->canSell());

  // One-unit prices delegate straight to the DB entry.
  QCOMPARE(i.buyPriceOneMoney(),  el->buyPriceMoney());
  QCOMPARE(i.buyPriceOneCoins(),  el->buyPriceCoins());
  QCOMPARE(i.sellPriceOneMoney(), el->sellPriceMoney());
  QCOMPARE(i.sellPriceOneCoins(), el->sellPriceCoins());

  // All-unit prices are one-unit * amount (amount = 3).
  QCOMPARE(i.buyPriceAllMoney(),  el->buyPriceMoney()  * 3);
  QCOMPARE(i.buyPriceAllCoins(),  el->buyPriceCoins()  * 3);
  QCOMPARE(i.sellPriceAllMoney(), el->sellPriceMoney() * 3);
  QCOMPARE(i.sellPriceAllCoins(), el->sellPriceCoins() * 3);
}

void TestItem::invalidItem_pricingIsZeroAndCannotSell()
{
  if(m_badInd < 0)
    QSKIP("every byte index 0..255 resolves to an item; cannot exercise the null guard");

  Item i(var8(m_badInd), var8(4));
  QVERIFY(i.toItem() == nullptr);

  QCOMPARE(i.canSell(), false);
  QCOMPARE(i.buyPriceOneMoney(),  0);
  QCOMPARE(i.buyPriceOneCoins(),  0);
  QCOMPARE(i.sellPriceOneMoney(), 0);
  QCOMPARE(i.sellPriceOneCoins(), 0);
  QCOMPARE(i.buyPriceAllMoney(),  0);
  QCOMPARE(i.sellPriceAllMoney(), 0);
}

void TestItem::setAmount_clampsToOneThrough99()
{
  Item i(var8(m_goodInd), var8(1));

  i.setAmount(50);
  QCOMPARE(i.getAmount(), 50); // in range, unchanged

  i.setAmount(0);
  QCOMPARE(i.getAmount(), 1);  // floor

  i.setAmount(200);
  QCOMPARE(i.getAmount(), 99); // ceiling
}

QTEST_GUILESS_MAIN(TestItem)
#include "tst_item.moc"
