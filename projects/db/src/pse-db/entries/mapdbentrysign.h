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
#include <QJsonValue>
#include "../db_autoport.h"

class MapDBEntry;
class QQmlEngine;
class MapDBEntry;

/**
 * @brief One sign defined on a map: its position and text id.
 *
 * The DB counterpart to the save's SignData. A leaf of MapDBEntry. See db.md.
 *
 * @see MapDBEntry (parent), SignData (the save-side sign).
 */
struct DB_AUTOPORT MapDBEntrySign : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getX READ getX CONSTANT)               ///< Sign tile X.
  Q_PROPERTY(int getY READ getY CONSTANT)               ///< Sign tile Y.
  Q_PROPERTY(int getTextID READ getTextID CONSTANT)     ///< Text id shown when read.
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT) ///< Owning map.

public:
  int getX() const;              ///< @see getX property.
  int getY() const;              ///< @see getY property.
  int getTextID() const;         ///< @see getTextID property.
  MapDBEntry* getParent() const; ///< @see getParent property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntrySign(); ///< Empty entry.
  MapDBEntrySign(const QJsonValue& data,
                 MapDBEntry* const parent); ///< Build from JSON under @p parent.
  void qmlRegister() const; ///< Register with QML.

  // X & Y location on Map
  int x = 0; ///< Sign tile X.
  int y = 0; ///< Sign tile Y.

  // Which text id to display when interacting with sign
  int textID = 0; ///< Text id shown when read.

  MapDBEntry* parent = nullptr; ///< Owning map.

  friend class MapDBEntry;
};
