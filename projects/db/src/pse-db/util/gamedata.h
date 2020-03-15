/*
  * Copyright 2019 June Hanabi
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
#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <QObject>
#include <QJsonDocument>
#include <QByteArray>

#include "../db_autoport.h"

class QQmlEngine;

// This helps with getting game data from the JSON files

class DB_AUTOPORT GameData : public QObject
{
  Q_OBJECT

public:
  // Get Instance
  static GameData* inst();

  // Retrieves JSON document from disk
  // Passed by value because of Qt's COW principle ensures no speed loss
  // Give the name of the file in /assets/data without the .json file extension
  const QByteArray jsonRaw(const QString filename) const;
  const QJsonDocument json(const QString filename) const;
  Q_INVOKABLE const QString jsonStr(const QString filename) const;

public slots:
  void engineProtect(const QQmlEngine* const engine) const;

private:
  GameData();

  void engineRegister() const;
};

#endif // GAMEDATA_H
