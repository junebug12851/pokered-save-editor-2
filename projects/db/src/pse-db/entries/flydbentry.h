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
class QQmlEngine;
class FlyDB;

// Qt 6 requires complete types for Q_PROPERTY pointers, OR this macro:

/**
 * @brief One fly destination: its name/index and the map it flies to.
 *
 * QObject-getter style DB entry (protected fields + getters). @ref toMap is
 * resolved in deepLink(). See db.md for the entry convention.
 *
 * @see FlyDB.
 */
struct DB_AUTOPORT FlyDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)       ///< Destination name.
  Q_PROPERTY(int     getInd  READ getInd  CONSTANT)       ///< Destination index.
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT) ///< Resolved destination map.

public:
  QString getName() const;     ///< @see getName property.
  int getInd() const;          ///< @see getInd property.
  MapDBEntry* getToMap() const; ///< @see getToMap property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  FlyDBEntry();                ///< Empty entry (built by FlyDB).
  FlyDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();             ///< Resolve the destination map.
  void qmlRegister() const;    ///< Register with QML.

  QString name = "";        ///< Backing field (read via getName()).
  int ind = 0;              ///< Backing field (read via getInd()).
  MapDBEntry* toMap = nullptr; ///< Backing field (read via getToMap()).

  friend class FlyDB; ///< The owning DB constructs/populates these entries.
};
