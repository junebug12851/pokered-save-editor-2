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
#include "abstractexample.h"

#include <QVector>
#include <QJsonArray>
#include <QtMath>
#include <QQmlEngine>

#include "../util/gamedata.h"
#include <pse-common/random.h>
#include <pse-common/utility.h>

void AbstractExample::load()
{
  store.clear();

  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json(fileName);

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Add to array
    store.append(jsonEntry.toString());
  }
}

void AbstractExample::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

AbstractExample::AbstractExample(QString fileName)
  : fileName(fileName)
{
  load();
}

const QVector<QString> AbstractExample::getStore() const
{
  return store;
}

int AbstractExample::getStoreSize() const
{
  return store.size();
}

const QString AbstractExample::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

QString AbstractExample::randomExample()
{
  int index = Random::inst()->rangeExclusive(0, store.size());
  QString ret = store.at(index);
  store.removeAt(index);

  if(store.size() == 0)
    load();

  return ret;
}
