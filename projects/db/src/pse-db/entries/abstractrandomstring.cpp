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
#include "abstractrandomstring.h"

#include <QVector>
#include <QJsonArray>
#include <QtMath>
#include <QQmlEngine>

#include "../util/gamedata.h"
#include <pse-common/random.h>
#include <pse-common/utility.h>

void AbstractRandomString::load()
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

  listChanged();
}

void AbstractRandomString::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

AbstractRandomString::AbstractRandomString(QString fileName)
  : fileName(fileName)
{
  load();
}

const QVector<QString> AbstractRandomString::getStore() const
{
  return store;
}

int AbstractRandomString::getStoreSize() const
{
  return store.size();
}

const QString AbstractRandomString::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

QString AbstractRandomString::randomExample()
{
  int index = Random::inst()->rangeExclusive(0, store.size());
  QString ret = store.at(index);
  store.removeAt(index);

  if(store.size() == 0)
    load();
  else
    listChanged();

  return ret;
}
