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

struct DB_AUTOPORT FlyDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(int     getInd  READ getInd  CONSTANT)
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT)

public:
  QString getName() const;
  int getInd() const;
  MapDBEntry* getToMap() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  FlyDBEntry();
  FlyDBEntry(QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  QString name = "";
  int ind = 0;
  MapDBEntry* toMap = nullptr;

  friend class FlyDB;
};
