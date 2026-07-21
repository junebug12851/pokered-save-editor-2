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
 * @file itemexchangemodel.cpp
 * @brief Implementation of ItemExchangeModel. See itemexchangemodel.h.
 *
 * The exchange is one net axis `m_net` (like the money<->coins converter's coin axis).
 * Positive net = gained-A steps, negative = gained-B. The WHOLE net is priced as a single
 * trade (giveFor/refundFor): the total value being bought is rounded up to a whole number
 * of the given item, and only that one leftover comes back as money. Rounding the total --
 * not each step -- is what makes an even trade free of change: 3 Fresh Water (₽600) costs
 * exactly 2 Potions (₽600) and refunds nothing.
 *
 * The after-counts and money for any candidate net are pure functions (aAfterFor/bAfterFor/
 * moneyAfterFor), which the "+" gating and the preview both use; checkout() writes exactly
 * that net via the box add/remove helpers (bag first, then PC storage).
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

// Healing sub-tab default SOURCE preference (what you give). Potion family first -- a
// player reaching for the Healing tab means potions long before Antidote.
const QStringList& healingSourcePref()
{
  static const QStringList s = {
    "POTION", "SUPER POTION", "HYPER POTION", "MAX POTION", "FULL RESTORE",
    "FULL HEAL", "REVIVE", "MAX REVIVE",
    "ELIXER", "MAX ELIXER", "ETHER", "MAX ETHER",
  };
  return s;
}

// Healing sub-tab default TARGET preference (what you get) -- the drinks, Fresh Water first.
const QStringList& healingTargetPref()
{
  static const QStringList s = { "FRESH WATER", "SODA POP", "LEMONADE" };
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
    in.name     = entry->getName();
    in.readable = entry->getReadable();
    in.buy      = entry->buyPriceMoney();
    in.healing  = healingNames().contains(in.name);
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

int ItemExchangeModel::indOfName(const QString& name) const
{
  for(auto it = m_info.constBegin(); it != m_info.constEnd(); ++it) {
    if(it->name == name)
      return it.key();
  }
  return -1;
}

void ItemExchangeModel::sortByName(QVariantList& list)
{
  QCollator col;
  col.setNumericMode(true);
  col.setCaseSensitivity(Qt::CaseInsensitive);
  std::sort(list.begin(), list.end(), [&col](const QVariant& a, const QVariant& b) {
    return col.compare(a.toMap().value("name").toString(),
                       b.toMap().value("name").toString()) < 0;
  });
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

// The whole trade is priced ONCE: round the TOTAL value up to a whole number of the given
// item, not each step separately. Steps that divide evenly therefore cost nothing extra and
// refund nothing -- 3 Fresh Water (3 x ₽200 = ₽600) is exactly 2 Potions (2 x ₽300 = ₽600).
int ItemExchangeModel::giveFor(int dir, int steps) const
{
  if(!valid() || steps <= 0 || dir == 0)
    return 0;
  return (dir > 0) ? ceilDiv(steps * aBuy(), bBuy())   // gaining A, paying in B
                   : ceilDiv(steps * bBuy(), aBuy());  // gaining B, paying in A
}

int ItemExchangeModel::refundFor(int dir, int steps) const
{
  if(!valid() || steps <= 0 || dir == 0)
    return 0;
  return (dir > 0) ? (giveFor(1, steps) * bBuy() - steps * aBuy())
                   : (giveFor(-1, steps) * aBuy() - steps * bBuy());
}

int ItemExchangeModel::aAfterFor(int m) const
{
  return aStart() + nA(m) - giveFor(-1, nB(m));
}

int ItemExchangeModel::bAfterFor(int m) const
{
  return bStart() + nB(m) - giveFor(1, nA(m));
}

int ItemExchangeModel::moneyAfterFor(int m) const
{
  return moneyStart() + refundFor(1, nA(m)) + refundFor(-1, nB(m));
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

// Everything that can move the state goes through here so `revision` (which the dropdown
// model bindings depend on) is always bumped BEFORE the binding re-reads it.
void ItemExchangeModel::emitChanged()
{
  ++m_revision;
  emit changed();
}

void ItemExchangeModel::setItemAInd(int v)
{
  if(v == m_itemAInd)
    return;
  m_itemAInd = v;
  m_net = 0;
  emit selectionChanged();
  emitChanged();
}

void ItemExchangeModel::setItemBInd(int v)
{
  if(v == m_itemBInd)
    return;
  m_itemBInd = v;
  m_net = 0;
  emit selectionChanged();
  emitChanged();
}

void ItemExchangeModel::adjust(int dir)
{
  if(dir > 0 && canAddA())
    ++m_net;
  else if(dir < 0 && canAddB())
    --m_net;
  else
    return;
  emitChanged();
}

void ItemExchangeModel::reset()
{
  if(m_net == 0)
    return;
  m_net = 0;
  emitChanged();
}

void ItemExchangeModel::refresh()
{
  clampNet();
  emitChanged();
}

// ---- Dropdown lists --------------------------------------------------------

// LEFT: what you can give -- the items you own. If you own nothing in this category the
// list would be empty (a dead dropdown), so it falls back to listing them all; no trade
// is possible either way, but the UI still reads as a normal (inert) pair.
QVariantList ItemExchangeModel::sourceItems(bool healingOnly, int excludeInd) const
{
  QVariantList out;
  for(int pass = 0; pass < 2 && out.isEmpty(); ++pass) {
    const bool ownedOnly = (pass == 0);
    for(auto it = m_info.constBegin(); it != m_info.constEnd(); ++it) {
      const int ind = it.key();
      if(ind == excludeInd)
        continue;
      if(it->buy <= 0)                            // must be exchangeable (has a buy value)
        continue;
      if(healingOnly && !it->healing)
        continue;
      if(ownedOnly && combinedAmount(ind) <= 0)   // owned, non-zero
        continue;
      QVariantMap m;
      m.insert("name", it->readable);
      m.insert("ind", ind);
      out.append(m);
    }
  }
  sortByName(out);
  return out;
}

// RIGHT: what you can get -- every exchangeable item, owned or not. `affordable` says
// whether your current source stock can actually cover one of it; the dropdown greys the
// ones that can't, so whatever IS selectable always leaves the "+" for it live.
QVariantList ItemExchangeModel::targetItems(bool healingOnly, int excludeInd) const
{
  QVariantList out;
  for(auto it = m_info.constBegin(); it != m_info.constEnd(); ++it) {
    const int ind = it.key();
    if(ind == excludeInd)
      continue;
    if(it->buy <= 0)
      continue;
    if(healingOnly && !it->healing)
      continue;
    QVariantMap m;
    m.insert("name", it->readable);
    m.insert("ind", ind);
    m.insert("affordable", canGainTarget(ind));
    out.append(m);
  }
  sortByName(out);
  return out;
}

// Evaluated against the START state (nothing is written until checkout), i.e. exactly the
// first "+<target>" step: enough of the source to cover it, room for the gained item, and
// the refund doesn't push money past the cap.
bool ItemExchangeModel::canGainTarget(int bInd) const
{
  if(m_itemAInd < 0 || bInd < 0 || bInd == m_itemAInd)
    return false;

  const int aB = buyOf(m_itemAInd);
  const int bB = buyOf(bInd);
  if(aB <= 0 || bB <= 0)
    return false;

  const int give   = ceilDiv(bB, aB);   // source items consumed for one target
  const int refund = give * aB - bB;    // leftover value, refunded as money

  if(combinedAmount(m_itemAInd) < give)      return false;
  if(combinedCapacity(bInd) < 1)             return false;
  if(moneyStart() + refund > MoneyCap)       return false;
  return true;
}

void ItemExchangeModel::pickDefaults(bool healingOnly)
{
  m_net = 0;
  m_itemAInd = -1;
  m_itemBInd = -1;

  // Source: on Healing, the best potion-family item the player actually HAS (Potion, then
  // Super/Hyper/Max, Full Restore, ... -- never Antidote while a potion is on hand).
  int a = -1;
  if(healingOnly) {
    for(const QString& n : healingSourcePref()) {
      const int ind = indOfName(n);
      if(ind >= 0 && buyOf(ind) > 0 && combinedAmount(ind) > 0) {
        a = ind;
        break;
      }
    }
  }
  if(a < 0) {
    const QVariantList src = sourceItems(healingOnly, -1);
    if(!src.isEmpty())
      a = src.first().toMap().value("ind").toInt();
  }
  m_itemAInd = a;   // canGainTarget() below reads this

  // Target: on Healing, Fresh Water (so the tab opens on Potion <=> Fresh Water). Fall
  // back to the first AFFORDABLE item so the pair we land on always has a live "+".
  int b = -1;
  if(healingOnly) {
    for(const QString& n : healingTargetPref()) {
      const int ind = indOfName(n);
      if(ind >= 0 && ind != a && canGainTarget(ind)) {
        b = ind;
        break;
      }
    }
  }
  if(b < 0) {
    const QVariantList tgt = targetItems(healingOnly, a);
    for(const QVariant& v : tgt) {
      const QVariantMap m = v.toMap();
      if(m.value("affordable").toBool()) {
        b = m.value("ind").toInt();
        break;
      }
    }
    // Nothing affordable at all (player owns nothing to trade) -- still show a sane pair.
    if(b < 0 && !tgt.isEmpty())
      b = tgt.first().toMap().value("ind").toInt();
  }
  m_itemBInd = b;

  emit selectionChanged();
  emitChanged();
}

void ItemExchangeModel::checkout()
{
  if(!valid() || m_net == 0)
    return;

  const int steps = (m_net > 0) ? m_net : -m_net;
  const int dir   = (m_net > 0) ? 1 : -1;

  // Priced as ONE trade (giveFor/refundFor), so this writes exactly what was previewed.
  const int give   = giveFor(dir, steps);
  const int refund = refundFor(dir, steps);

  const int gainInd = (dir > 0) ? m_itemAInd : m_itemBInd;
  const int giveInd = (dir > 0) ? m_itemBInd : m_itemAInd;

  // Consume the given item (bag first, then PC storage), add the gained one, refund the
  // leftover value as money.
  int rem = bag ? bag->removeAmount(giveInd, give) : 0;
  if(rem < give && storage)
    storage->removeAmount(giveInd, give - rem);

  int added = bag ? bag->addAmount(gainInd, steps) : 0;
  if(added < steps && storage)
    storage->addAmount(gainInd, steps - added);

  if(basics) {
    const int m = static_cast<int>(basics->money) + refund;
    basics->money = static_cast<unsigned int>(m > MoneyCap ? MoneyCap : m);
  }

  if(basics)
    basics->moneyChanged();

  m_net = 0;
  // The box itemsChanged/moneyChanged signals already trigger refresh(); emit anyway
  // so the preview settles even if a box somehow emitted nothing.
  emitChanged();
}
