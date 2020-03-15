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
#include "gamedata.h"
#include "../db.h"

#include <QFile>
#include <QByteArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

const QByteArray GameData::jsonRaw(const QString filename) const
{
  // Prepare variables
  QByteArray val;
  QFile file;

  // Read in file
  file.setFileName(":/assets/data/" + filename + ".json");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  val = file.readAll();
  file.close();

  return val;
}

const QJsonDocument GameData::json(const QString filename) const
{
  // Convert to JSON Document
  return QJsonDocument(QJsonDocument::fromJson(jsonRaw(filename)));
}

const QString GameData::jsonStr(const QString filename) const
{
  return jsonRaw(filename);
}

void GameData::engineProtect(const QQmlEngine* const engine) const
{
  Utility::engineProtectUtil(this, engine);
}

GameData::GameData()
{
  engineRegister();
}

void GameData::engineRegister() const
{
  qmlRegisterUncreatableType<GameData>("PSE.DB.GameData", 1, 0, "GameData", "Can't instantiate in QML");
}

GameData* GameData::inst()
{
  static GameData* _inst = new GameData;
  return _inst;
}
