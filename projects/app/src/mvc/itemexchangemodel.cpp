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
 * @file itemexchangemodel.cpp
 * @brief Implementation of ItemExchangeModel. See itemexchangemodel.h.
 *
 * The exchange is one net axis `m_net` (like the money<->coins converter's coin axis).
 * Positive net = gained-A steps, negative = gained-B. Per step: gain 1 of the target,
 * consume the minimum whole number of the other item whose BUY value covers the target,
 * and refund the leftover value as money. The after-counts and money for any candidate
 * net are pure functions (aAfterFor/bAfterFor/moneyAfterFor), which the "+" gating and
 * the preview both use; checkout() writes exactly that net via the box add/remove
 * helpers (bag first, then PC storage).
 */

#include "./itemexchangemodel.h"

#include <QCollator>
#include <QSet>
#include <algorithm>

#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>

namespace {
// Curated healing / status / drink item set for the Healing sub-tab (internal names
// from items.json). UI filter only -- not save data.
const QSet<QString>& healingNames()
{
  static const QSet<QString> s = {
    "POTION", "SUPER POTION", "HYPER POTION", "MAX POTION", "FULL RESTORE",
    "FULL HEAL", "REVIVE", "MAX REVIVE", "ANTIDOTE", "BURN HEAL", "ICE HEAL",
    "AWAKENING", "PARLYZ HEAL", "ETHER", "MAX ETHER", "ELIXER", "MAX ELIXER",
    "FRESH WATER", "SODA POP", "LEMONADE",
  };
  return s;
}

int ceilDiv(int a, int b) { return (b <= 0) ? 0 : ((a + b - 1) / b); }
}

ItemExchangeModel::ItemExchangeModel(ItemStorageBox* bag,
                                     ItemStorageBox* storage,
                                     PlayerBasics* basics)
  : bag(bag), storage(storage), basics(basics)
{
  // Cache each item's static info once (readable name, money buy price, healing flag).
  for(auto entry : ItemsDB::inst()->getStore()) {
    const int ind = entry->getInd();
    if(ind < 0)
      continue;
    Info in;
    in.readable = entry->getReadable();
    in.buy      = entry->buyPriceMoney();
    in.healing  = healingNames().contains(entry->getName());
    m_info.insert(ind, in);
  }

  // Keep the preview live as the save changes under us (checkout, or edits elsewhere).
  if(bag)     QObject::connect(bag,     &ItemStorageBox::itemsChanged, this, &ItemExchangeModel::refresh);
  if(storage) QObject::connect(storage, &ItemStorageBox::itemsChanged, this, &ItemExchangeModel::refresh);
  if(basics) {
    QObject::connect(basics, &PlayerBasics::moneyChanged, this, &ItemExchangeModel::refresh);
    QObject::connect(basics, &PlayerBasics::coinsChanged, this, &ItemExchangeModel::refresh);
  }
}

// ---- Static per-item lookups (from the cache) ------------------------------

int ItemExchangeModel::buyOf(int ind) const
{
  auto it = m_info.constFind(ind);
  return (it == m_info.constEnd()) ? 0 : it->buy;
}

QString ItemExchangeModel::readableOf(int ind) const
{
  auto it = m_info.constFind(ind);
  return (it == m_info.constEnd()) ? QString() : it->readable;
}

bool ItemExchangeModel::isHealing(int ind) const
{
  auto it = m_info.constFind(ind);
  return (it != m_info.constEnd()) && it->healing;
}

// ---- Live save reads -------------------------------------------------------

int ItemExchangeModel::combinedAmount(int ind) const
{
  if(ind < 0)
    return 0;
  int total = 0;
  if(bag)     total += bag->amountOfInd(ind);
  if(storage) total += storage->amountOfInd(ind);
  return total;
}

int ItemExchangeModel::combinedCapacity(int ind) const
{
  if(ind < 0)
    return 0;
  int room = 0;
  if(bag)     room += bag->capacityForInd(ind);
  if(storage) room += storage->capacityForInd(ind);
  return room;
}

int ItemExchangeModel::moneyStart() const
{
  return basics ? static_cast<int>(basics->money) : 0;
}

// ---- Exchange math ---------------------------------------------------------

bool ItemExchangeModel::valid() const
{
  return m_itemAInd >= 0 && m_itemBInd >= 0
      && m_itemAInd != m_itemBInd
      && aBuy() > 0 && bBuy() > 0;
}

int ItemExchangeModel::perAGive() const   { return valid() ? ceilDiv(aBuy(), bBuy()) : 0; }
int ItemExchangeModel::perARefund() const { return valid() ? (perAGive() * bBuy() - aBuy()) : 0; }
int ItemExchangeModel::perBGive() const   { return valid() ? ceilDiv(bBuy(), aBuy()) : 0; }
int ItemExchangeModel::perBRefund() const { return valid() ? (perBGive() * aBuy() - bBuy()) : 0; }

int ItemExchangeModel::aAfterFor(int m) const
{
  return aStart() + nA(m) - nB(m) * perBGive();
}

int ItemExchangeModel::bAfterFor(int m) const
{
  return bStart() + nB(m) - nA(m) * perAGive();
}

int ItemExchangeModel::moneyAfterFor(int m) const
{
  return moneyStart() + nA(m) * perARefund() + nB(m) * perBRefund();
}

// A net value is legal when nothing goes negative, the gained item fits, and money
// stays under the cap.
bool ItemExchangeModel::stateValid(int m) const
{
  if(!valid())
    return m == 0;

  const int aA = aAfterFor(m);
  const int bA = bAfterFor(m);
  const int moneyA = moneyAfterFor(m);

  if(aA < 0 || bA < 0)
    return false;
  if(aA > aStart() + combinedCapacity(m_itemAInd))
    return false;
  if(bA > bStart() + combinedCapacity(m_itemBInd))
    return false;
  if(moneyA > MoneyCap)
    return false;

  return true;
}

void ItemExchangeModel::clampNet()
{
  while(m_net > 0 && !stateValid(m_net)) --m_net;
  while(m_net < 0 && !stateValid(m_net)) ++m_net;
}

// ---- Mutations -------------------------------------------------------------

void ItemExchangeModel::setItemAInd(int v)
{
  if(v == m_itemAInd)
    return;
  m_itemAInd = v;
  m_net = 0;
  emit selectionChanged();
  emit changed();
}

void ItemExchangeModel::setItemBInd(int v)
{
  if(v == m_itemBInd)
    return;
  m_itemBInd = v;
  m_net = 0;
  emit selectionChanged();
  emit changed();
}

void ItemExchangeModel::adjust(int dir)
{
  if(dir > 0 && canAddA())
    ++m_net;
  else if(dir < 0 && canAddB())
    --m_net;
  else
    return;
  emit changed();
}

void ItemExchangeModel::reset()
{
  if(m_net == 0)
    return;
  m_net = 0;
  emit changed();
}

void ItemExchangeModel::refresh()
{
  clampNet();
  emit changed();
}

QVariantList ItemExchangeModel::ownedItems(bool healingOnly, int excludeInd) const
{
  QVariantList out;
  for(auto it = m_info.constBegin(); it != m_info.constEnd(); ++it) {
    const int ind = it.key();
    if(ind == excludeInd)
      continue;
    if(it->buy <= 0)               // must be exchangeable (has a buy value)
      continue;
    if(healingOnly && !it->healing)
      continue;
    if(combinedAmount(ind) <= 0)   // owned, non-zero
      continue;
    QVariantMap m;
    m.insert("name", it->readable);
    m.insert("ind", ind);
    out.append(m);
  }

  // Sort by display name (same collation feel as the other item lists).
  QCollator col;
  col.setNumericMode(true);
  col.setCaseSensitivity(Qt::CaseInsensitive);
  std::sort(out.begin(), out.end(), [&col](const QVariant& a, const QVariant& b) {
    return col.compare(a.toMap().value("name").toString(),
                       b.toMap().value("name").toString()) < 0;
  });
  return out;
}

void ItemExchangeModel::checkout()
{
  if(!valid() || m_net == 0)
    return;

  const int steps = (m_net > 0) ? m_net : -m_net;

  if(m_net > 0) {
    // Gained A: consume B (bag first, then storage), then add A, then refund money.
    const int giveB = steps * perAGive();
    int rem = bag ? bag->removeAmount(m_itemBInd, giveB) : 0;
    if(rem < giveB && storage)
      storage->removeAmount(m_itemBInd, giveB - rem);

    int added = bag ? bag->addAmount(m_itemAInd, steps) : 0;
    if(added < steps && storage)
      storage->addAmount(m_itemAInd, steps - added);

    if(basics) {
      const int m = static_cast<int>(basics->money) + steps * perARefund();
      basics->money = static_cast<unsigned int>(m > MoneyCap ? MoneyCap : m);
    }
  } else {
    // Gained B: consume A, add B, refund money.
    const int giveA = steps * perBGive();
    int rem = bag ? bag->removeAmount(m_itemAInd, giveA) : 0;
    if(rem < giveA && storage)
      storage->removeAmount(m_itemAInd, giveA - rem);

    int added = bag ? bag->addAmount(m_itemBInd, steps) : 0;
    if(added < steps && storage)
      storage->addAmount(m_itemBInd, steps - added);

    if(basics) {
      const int m = static_cast<int>(basics->money) + steps * perBRefund();
      basics->money = static_cast<unsigned int>(m > MoneyCap ? MoneyCap : m);
    }
  }

  if(basics)
    basics->moneyChanged();

  m_net = 0;
  // The box itemsChanged/moneyChanged signals already trigger refresh(); emit anyway
  // so the preview settles even if a box somehow emitted nothing.
  emit changed();
}
