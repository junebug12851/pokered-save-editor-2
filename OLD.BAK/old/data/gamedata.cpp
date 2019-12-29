/*
Copyright 2019 June Hanabi

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "gamedata.h"

#include <QFile>
#include <QByteArray>

GameData::GameData(QObject *parent) : QObject(parent)
{}

QString GameData::json(QString filename)
{
  // Serve cached file if present
  if(cache->contains(filename))
    return cache->value(filename);

  // Prepare variables
  QByteArray val;
  QFile file;

  // Read in file
  file.setFileName(":/assets/data/" + filename + ".json");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  val = file.readAll();
  file.close();

  // Cache read file and then serve
  cache->insert(filename, val);
  return val;
}

QObject* GameData::GameData_Provider(QQmlEngine* engine, QJSEngine* scriptEngine)
{
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)

  return new GameData();
}

QHash<QString, QString>* GameData::cache = new QHash<QString, QString>();