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
#ifndef HIDDENITEMDBENTRY_H
#define HIDDENITEMDBENTRY_H

#include <QObject>
#include <QString>
#include <QJsonValue>

#include "../db_autoport.h"

struct MapDBEntry;
class AbstractHiddenItemDB;
class QQmlEngine;

// A list of all the hidden coins in Casino

struct DB_AUTOPORT HiddenItemDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getMap READ getMap CONSTANT)
  Q_PROPERTY(int getX READ getX CONSTANT)
  Q_PROPERTY(int getY READ getY CONSTANT)
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT)

public:
  const QString getMap() const;
  int getX() const;
  int getY() const;
  const MapDBEntry* getToMap() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  HiddenItemDBEntry();
  HiddenItemDBEntry(const QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  QString map = "";
  int x = 0;
  int y = 0;

  MapDBEntry* toMap = nullptr;

  friend class AbstractHiddenItemDB;
};

#endif // HIDDENITEMDBENTRY_H
