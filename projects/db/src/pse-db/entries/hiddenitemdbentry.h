/*
  * Copyright 2020 Fairy Fox
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
#include <QJsonValue>
#include "../db_autoport.h"

struct MapDBEntry;
class AbstractHiddenItemDB;
class QQmlEngine;


/**
 * @brief One hidden pickup: what is buried, and where.
 *
 * QObject-getter style DB entry shared by both hidden databases (items and coins,
 * via AbstractHiddenItemDB). @ref toMap is resolved in deepLink(). See db.md.
 *
 * **The entry's index IS the save bit.** `HiddenItems` -> `FindHiddenItemOrCoinsIndex` tests
 * bit `n` of `wObtainedHiddenItemsFlags`, where `n` is the row's position in the ROM's
 * `HiddenItemCoords`. So store index `i` == WorldHidden's `hiddenItems[i]` == this (map, x, y).
 * That is index identity, not inference -- which is what makes these 66 pickups placeable on
 * the map exactly, with nothing guessed.
 *
 * @ref x / @ref y are on the **walk grid** (half-blocks, 16x16px) -- the same grid as
 * `wYCoord`/`wXCoord`, warps, signs and objects. Not tiles (8x8), not blocks (32x32).
 *
 * Only one of @ref item / @ref coins is meaningful per entry, decided by which DB owns it:
 * HiddenItemsDB entries carry @ref item, HiddenCoinsDB entries carry @ref coins. The two share
 * this class because they share their shape, and the ROM stores both the same way.
 *
 * @see AbstractHiddenItemDB, HiddenItemsDB, HiddenCoinsDB, WorldHidden.
 * @see notes/reference/map-storage-locations.md
 */
struct DB_AUTOPORT HiddenItemDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(int         getInd   READ getInd   CONSTANT) ///< Save bit index (== store index).
  Q_PROPERTY(bool        getIsCoin READ getIsCoin CONSTANT) ///< Coin pile (true) or item (false)?
  Q_PROPERTY(QString     getMap   READ getMap   CONSTANT) ///< Map name the pickup is on.
  Q_PROPERTY(int         getX     READ getX     CONSTANT) ///< Walk-grid X.
  Q_PROPERTY(int         getY     READ getY     CONSTANT) ///< Walk-grid Y.
  Q_PROPERTY(QString     getItem  READ getItem  CONSTANT) ///< Item buried here (items DB only).
  Q_PROPERTY(int         getCoins READ getCoins CONSTANT) ///< Coins buried here (coins DB only).
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT) ///< Resolved map.

public:
  /// This pickup's index in its own DB's store -- which IS its bit in the save. Item entries
  /// index WorldHidden::hiddenItems, coin entries index WorldHidden::hiddenCoins; the two are
  /// separate arrays with independent numbering, so this is only meaningful together with
  /// @ref getIsCoin.
  int        getInd()   const;
  bool       getIsCoin() const; ///< @see getIsCoin property.
  QString    getMap()   const; ///< @see getMap property.
  int        getX()     const; ///< @see getX property.
  int        getY()     const; ///< @see getY property.
  /// The buried item's name, spelled exactly as items.json spells it ("GREAT BALL") -- so it is
  /// both readable and a valid ItemsDB key. Empty for a coin entry.
  QString    getItem()  const;
  /// How many coins are buried here. 0 for an item entry.
  int        getCoins() const;
  MapDBEntry* getToMap() const; ///< @see getToMap property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  HiddenItemDBEntry();                    ///< Empty entry (built by the DB).
  /// @param data the JSON row.
  /// @param ind this row's index -- the save bit it owns.
  /// @param isCoin true if the owning DB is HiddenCoinsDB.
  HiddenItemDBEntry(const QJsonValue& data, int ind, bool isCoin);
  void deepLink();                        ///< Resolve the map link.
  void qmlRegister() const;               ///< Register with QML.

  int ind = -1;            ///< Backing field (read via getInd()). This entry's save bit.
  bool isCoin = false;     ///< Backing field (read via getIsCoin()).
  QString map = "";        ///< Backing field (read via getMap()).
  int x = 0;               ///< Backing field (read via getX()).
  int y = 0;               ///< Backing field (read via getY()).
  QString item = "";       ///< Backing field (read via getItem()). Empty on a coin entry.
  int coins = 0;           ///< Backing field (read via getCoins()). 0 on an item entry.
  MapDBEntry* toMap = nullptr; ///< Resolved map (deepLink).

  friend class AbstractHiddenItemDB; ///< Owning DB constructs/populates entries.
};
