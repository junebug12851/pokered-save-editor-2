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
#pragma once
#include <QObject>
#include <QString>
#include <QHash>
#include <QVariantList>

class ItemStorageBox;
class PlayerBasics;

/**
 * @brief Item<->item exchange for the Market's Exchange tab (Healing + Custom).
 *
 * Powers the two item-trading sub-tabs. Two items are selected (A and B); a single
 * net axis (@ref net) drives the trade the same way the money<->coins converter drives
 * one coin axis. Each "+A" step gains ONE of item A and draws the minimum whole number
 * of item B whose BUY value covers A's buy value, refunding the leftover value as money
 * (money only ever goes up); "+B" is the mirror. Everything is previewed (the after
 * counts + money) and only written on checkout(). Values come from each item's money
 * buy price; the traded items are counted across the bag + PC storage combined.
 *
 * The two dropdowns are asymmetric: the LEFT one is what you GIVE, so it lists the items you
 * actually own (@ref sourceItems); the RIGHT one is what you GET, so it lists EVERY exchangeable
 * item (@ref targetItems) with the ones your stock can't cover flagged unaffordable and greyed.
 * That greying is what keeps a selected pair always tradeable in at least one direction -- the
 * two "+" buttons are never both disabled. Healing filters both lists to healing/drink items and
 * starts on Potion <=> Fresh Water (@ref pickDefaults). Exposed as `brg.itemExchangeModel`.
 *
 * @see ItemStorageBox (addAmount/removeAmount/capacityForInd), ItemMarketModel (the
 *      money<->coins exchange this mirrors).
 */
class ItemExchangeModel : public QObject
{
  Q_OBJECT

  // The two selected item indices (-1 = none). The dropdowns write these.
  Q_PROPERTY(int itemAInd READ itemAInd WRITE setItemAInd NOTIFY selectionChanged)
  Q_PROPERTY(int itemBInd READ itemBInd WRITE setItemBInd NOTIFY selectionChanged)

  Q_PROPERTY(bool valid READ valid NOTIFY changed)          ///< Both picked, distinct, both have a buy price.
  Q_PROPERTY(int net READ net NOTIFY changed)               ///< Net axis (+N = N gained-A steps, -N = gained-B).

  Q_PROPERTY(QString aName READ aName NOTIFY changed)       ///< Item A display name.
  Q_PROPERTY(QString bName READ bName NOTIFY changed)       ///< Item B display name.
  Q_PROPERTY(int aBuy READ aBuy NOTIFY changed)             ///< Item A buy price (money).
  Q_PROPERTY(int bBuy READ bBuy NOTIFY changed)             ///< Item B buy price (money).

  Q_PROPERTY(int aStart READ aStart NOTIFY changed)         ///< Item A owned (bag+storage) before.
  Q_PROPERTY(int aAfter READ aAfter NOTIFY changed)         ///< Item A owned after the previewed trade.
  Q_PROPERTY(int bStart READ bStart NOTIFY changed)         ///< Item B owned before.
  Q_PROPERTY(int bAfter READ bAfter NOTIFY changed)         ///< Item B owned after.
  Q_PROPERTY(int moneyStart READ moneyStart NOTIFY changed) ///< Money before.
  Q_PROPERTY(int moneyAfter READ moneyAfter NOTIFY changed) ///< Money after (refunds only).

  Q_PROPERTY(int perAGive READ perAGive NOTIFY changed)     ///< Item B consumed per +A step.
  Q_PROPERTY(int perARefund READ perARefund NOTIFY changed) ///< Money refunded per +A step.
  Q_PROPERTY(int perBGive READ perBGive NOTIFY changed)     ///< Item A consumed per +B step.
  Q_PROPERTY(int perBRefund READ perBRefund NOTIFY changed) ///< Money refunded per +B step.

  Q_PROPERTY(bool canAddA READ canAddA NOTIFY changed)      ///< "+A" enabled (one more gained-A step is legal).
  Q_PROPERTY(bool canAddB READ canAddB NOTIFY changed)      ///< "+B" enabled.

  /// Bumped on every change. sourceItems()/targetItems() are Q_INVOKABLE calls, so a QML
  /// `model:` binding on them has no dependency to re-run on -- read this in the binding
  /// and the lists (and their `affordable` flags) rebuild whenever the state moves.
  Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
  static constexpr int MoneyCap = 999999; ///< Gen 1 money cap.
  static constexpr int StackCap = 99;     ///< Gen 1 per-stack cap.

  ItemExchangeModel(ItemStorageBox* bag, ItemStorageBox* storage, PlayerBasics* basics);

  int itemAInd() const { return m_itemAInd; }
  int itemBInd() const { return m_itemBInd; }
  void setItemAInd(int v);
  void setItemBInd(int v);

  int net() const { return m_net; }
  int revision() const { return m_revision; }
  bool valid() const;

  QString aName() const { return readableOf(m_itemAInd); }
  QString bName() const { return readableOf(m_itemBInd); }
  int aBuy() const { return buyOf(m_itemAInd); }
  int bBuy() const { return buyOf(m_itemBInd); }

  int aStart() const { return combinedAmount(m_itemAInd); }
  int bStart() const { return combinedAmount(m_itemBInd); }
  int moneyStart() const;
  int aAfter() const { return aAfterFor(m_net); }
  int bAfter() const { return bAfterFor(m_net); }
  int moneyAfter() const { return moneyAfterFor(m_net); }

  int perAGive() const;
  int perARefund() const;
  int perBGive() const;
  int perBRefund() const;

  bool canAddA() const { return valid() && stateValid(m_net + 1); }
  bool canAddB() const { return valid() && stateValid(m_net - 1); }

  /// LEFT list -- what you can GIVE: items you actually own (amount > 0) and that are
  /// exchangeable (buy price > 0), optionally healing-only, excluding @p excludeInd.
  /// Returns [{name, ind}] sorted by display name.
  Q_INVOKABLE QVariantList sourceItems(bool healingOnly, int excludeInd) const;

  /// RIGHT list -- what you can GET: EVERY exchangeable item (owned or not), optionally
  /// healing-only, excluding @p excludeInd. Each entry carries an `affordable` flag --
  /// false when your current source stock can't cover even one of it (or there's no room
  /// / money would overflow). The dropdown greys those, which is what guarantees the
  /// selected pair always has at least one usable "+" (never both disabled).
  /// Returns [{name, ind, affordable}] sorted by display name.
  Q_INVOKABLE QVariantList targetItems(bool healingOnly, int excludeInd) const;

  /// Can we gain at least one of item @p bInd right now, paying with the selected source?
  Q_INVOKABLE bool canGainTarget(int bInd) const;

  /// Choose a sensible starting pair. Healing prefers a potion-family source the player
  /// owns (Potion > Super > Hyper > Max > Full Restore, then any healing item) and Fresh
  /// Water as the target -- i.e. Potion <=> Fresh Water by default. Always lands on a
  /// pair with a usable "+" when one exists.
  Q_INVOKABLE void pickDefaults(bool healingOnly);

  Q_INVOKABLE void adjust(int dir); ///< +1 = one gained-A step, -1 = one gained-B step (gated).
  Q_INVOKABLE void reset();         ///< Clear the net axis.
  Q_INVOKABLE void refresh();       ///< Re-read counts/money + re-emit (tab open / external edit).

public slots:
  void checkout(); ///< Apply the previewed trade to the save.

signals:
  void selectionChanged(); ///< A or B selection changed.
  void changed();          ///< Any derived value changed.

private:
  int combinedAmount(int ind) const;   ///< Owned across bag + storage.
  int combinedCapacity(int ind) const; ///< Room to add across bag + storage.
  int buyOf(int ind) const;            ///< Money buy price, 0 if none / unknown.
  QString readableOf(int ind) const;   ///< Display name, "" if unknown.
  bool isHealing(int ind) const;       ///< In the curated healing/drink set?
  int indOfName(const QString& name) const;      ///< Item index by INTERNAL name, -1 if unknown.
  static void sortByName(QVariantList& list);    ///< Sort [{name,...}] by display name.

  static int nA(int m) { return m > 0 ? m : 0; }
  static int nB(int m) { return m < 0 ? -m : 0; }
  int aAfterFor(int m) const;
  int bAfterFor(int m) const;
  int moneyAfterFor(int m) const;
  bool stateValid(int m) const;        ///< Is net==m a legal state (counts/caps/money)?
  void clampNet();                     ///< Pull net back into the legal range.
  void emitChanged();                  ///< Bump @ref revision, then emit changed().

  ItemStorageBox* bag = nullptr;
  ItemStorageBox* storage = nullptr;
  PlayerBasics* basics = nullptr;

  int m_itemAInd = -1;
  int m_itemBInd = -1;
  int m_net = 0;
  int m_revision = 0;

  /// Static per-item info cached at construction (ind -> internal / readable / buy / healing).
  struct Info { QString name; QString readable; int buy = 0; bool healing = false; };
  QHash<int, Info> m_info;
};
