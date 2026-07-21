/*
  * Copyright 2019 Fairy Fox
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
#include <QJsonValue>
#include <QString>
#include <QVector>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct PokemonDBEntry;
struct MapDBEntry;
class QQmlEngine;

/**
 * @brief One in-game (NPC) trade definition: what you give and get, and where.
 *
 * Plain-struct DB entry. @ref toGive / @ref toGet / @ref toMap resolve in deepLink(). See db.md for
 * the entry convention. Loaded from trades.json, whose location fields (@ref ind .. @ref walks) are
 * generated from pret by scripts/import_trades.py; the give/get/textId/nickname/unused half predates
 * that and is kept verbatim.
 *
 * **Store index == save bit == @ref ind.** `DoInGameTradeDialogue` FLAG_TESTs bit `wWhichTrade` of
 * `wCompletedInGameTradeFlags` (file 0x29E3), and the store is loaded in bit order, so entry `i` is
 * trade `i`. The flag is the ONLY gate: set, the trader only speaks their after-trade line; clear,
 * the trade is genuinely re-armed.
 *
 * ⚠️ @ref textId is an INT and is really the DIALOG SET (0 casual / 1 evolution / 2 happy), NOT a
 * text id -- an inherited misnomer, kept because renaming a shipped field is a separate decision.
 * `"evolution"` does not mean the mon evolves (a Japanese Blue leftover; LOLA's and DORIS's traders
 * claim your Poliwhirl/Raichu "evolved" about species that cannot).
 *
 * ⚠️ @ref nickname is the RECEIVED mon's nickname -- NOT the trader's name. TERRY is the Nidorina.
 * Traders have no personal name at all; @ref trader is their sprite CLASS, which repeats across
 * trades (two Youngsters, two Little Girls, two Scientists).
 *
 * ⚠️ The unused trade (CHIKUCHIKU) has no script, so no NPC, so no map: @ref mapId is -1, @ref toMap
 * is null, and it belongs on the General page. @ref x / @ref y are the walk grid (16x16), and are
 * the trader's SPAWN tile when @ref walks is true.
 *
 * @see TradesDB, WorldTrades (the save-side flags), notes/reference/in-game-trades.md.
 */
struct DB_AUTOPORT TradeDBEntry {
  TradeDBEntry();                ///< Empty entry.
  TradeDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();              ///< Resolve the give/get species + map links.

  QString give;        ///< Species name you give (resolved to @ref toGive).
  QString get;         ///< Species name you get (resolved to @ref toGet).
  var8    textId   = 0; ///< ⚠️ The DIALOG SET (0/1/2), not a text id -- see the class note.
  QString nickname;    ///< Nickname the RECEIVED mon comes with (not the trader's name).
  bool    unused   = false; ///< Whether this trade slot is unused (CHIKUCHIKU only).

  // --- location, appended by import_trades.py (additive; the above predate it) ---
  int     ind      = -1; ///< Save bit in wCompletedInGameTradeFlags (== store index).
  QString tradeConst;    ///< pret's TRADE_FOR_* constant.
  int     mapId    = -1; ///< Map id the trader stands on; -1 for the unused trade.
  int     x        = 0;  ///< Walk-grid X (spawn tile if @ref walks).
  int     y        = 0;  ///< Walk-grid Y (spawn tile if @ref walks).
  QString trader;        ///< The trader's sprite CLASS name (they have no personal name).
  bool    walks    = false; ///< Trader wanders -> x/y is the spawn tile only.

  PokemonDBEntry* toGive = nullptr; ///< Resolved species you give (deepLink).
  PokemonDBEntry* toGet  = nullptr; ///< Resolved species you get (deepLink).
  MapDBEntry*     toMap  = nullptr; ///< Resolved map (deepLink); null for the unused trade.
};

/**
 * @brief The in-game trades database.
 *
 * Standard DB-singleton (no key index; trades are accessed by store index). See
 * db.md.
 *
 * @see TradeDBEntry, DB.
 */
class DB_AUTOPORT TradesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of trades.

public:
  static TradesDB* inst(); ///< The process-wide TradesDB singleton.

  [[nodiscard]] const QVector<TradeDBEntry*> getStore() const; ///< All trades.
  [[nodiscard]] int getStoreSize() const;                      ///< Trade count.

  Q_INVOKABLE TradeDBEntry* getStoreAt(int idx) const;         ///< Trade by store index (for QML).

public slots:
  void load();     ///< Load trades from JSON.
  void deepLink(); ///< Resolve each trade's species links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  TradesDB(); ///< Private -- use inst().

  QVector<TradeDBEntry*> store; ///< The loaded trades.
};
