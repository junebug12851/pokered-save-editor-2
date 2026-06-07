/*
  * Copyright 2020 Twilight
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
 * @brief One hidden pickup's location: its map and tile coordinates.
 *
 * QObject-getter style DB entry shared by both hidden databases (items and coins,
 * via AbstractHiddenItemDB). @ref toMap is resolved in deepLink(). See db.md.
 *
 * @see AbstractHiddenItemDB, HiddenItemsDB, HiddenCoinsDB, WorldHidden.
 */
struct DB_AUTOPORT HiddenItemDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString     getMap   READ getMap   CONSTANT) ///< Map name the pickup is on.
  Q_PROPERTY(int         getX     READ getX     CONSTANT) ///< Tile X.
  Q_PROPERTY(int         getY     READ getY     CONSTANT) ///< Tile Y.
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT) ///< Resolved map.

public:
  QString    getMap()   const; ///< @see getMap property.
  int        getX()     const; ///< @see getX property.
  int        getY()     const; ///< @see getY property.
  MapDBEntry* getToMap() const; ///< @see getToMap property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  HiddenItemDBEntry();                    ///< Empty entry (built by the DB).
  HiddenItemDBEntry(const QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                        ///< Resolve the map link.
  void qmlRegister() const;               ///< Register with QML.

  QString map = "";        ///< Backing field (read via getMap()).
  int x = 0;               ///< Backing field (read via getX()).
  int y = 0;               ///< Backing field (read via getY()).
  MapDBEntry* toMap = nullptr; ///< Resolved map (deepLink).

  friend class AbstractHiddenItemDB; ///< Owning DB constructs/populates entries.
};
