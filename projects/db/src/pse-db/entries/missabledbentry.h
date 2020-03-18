/*
  * Copyright 2020 June Hanabi
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
#ifndef MISSABLEDBENTRY_H
#define MISSABLEDBENTRY_H

#include <QObject>
#include <QJsonValue>
#include <QString>
#include <QHash>

#include "../db_autoport.h"

struct MapDBEntry;
struct MapDBEntrySprite;
class QQmlEngine;
class MissablesDB;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Missable flags set in-game, a missable is simply a script and/or sprite
// that never loads (Is surpressed). Allows the game to hide things you
// shouldn't see or encounter yet or show a new map "state" after you progressed
// in the games.

// The starter you and your rival pick are both missable activated and the
// guy blocking the path in Pewter City is a missable that's hiden once you beat
// Brock.

struct DB_AUTOPORT MissableDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(int getInd READ getInd CONSTANT)
  Q_PROPERTY(QString getMap READ getMap CONSTANT)
  Q_PROPERTY(int getSprite READ getSprite CONSTANT)
  Q_PROPERTY(bool getDefShow READ getDefShow CONSTANT)
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT)
  Q_PROPERTY(MapDBEntrySprite* getToMapSprite READ getToMapSprite CONSTANT)

public:
  const QString getName() const;
  int getInd() const;
  const QString getMap() const;
  int getSprite() const;
  bool getDefShow() const;
  const MapDBEntry* getToMap() const;
  const MapDBEntrySprite* getToMapSprite() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  MissableDBEntry();
  MissableDBEntry(QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  // Missable Name & Index
  QString name = "";
  int ind = 0;

  // Map & Sprite on map Missable References
  QString map = "";
  int sprite = 0;

  // Is this missable shown or hidden by default
  bool defShow = false;

  // Deep link to associated map and sprite on map
  // There are 2 exceptions to this
  // * There's one missable that references a sprite on an incomplete map with
  //   no sprites
  // * There's one missable that references an extra sprite which isn't there
  //
  // In both cases one or both of these will be nullptr
  MapDBEntry* toMap = nullptr;
  MapDBEntrySprite* toMapSprite = nullptr;

  friend class MissablesDB;
};

#endif // MISSABLEDBENTRY_H
