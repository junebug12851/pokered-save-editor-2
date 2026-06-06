/*
  * Copyright 2019 Twilight
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

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>
#include <pse-common/random.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./starterPokemon.h"
#include "./util/gamedata.h"
#include "./pokemon.h"

StarterPokemonDB* StarterPokemonDB::inst()
{
  static StarterPokemonDB* _inst = new StarterPokemonDB;
  return _inst;
}

int StarterPokemonDB::getStoreSize() const { return store.size(); }

PokemonDBEntry* StarterPokemonDB::random3Starter() const
{
  var32 idx = Random::inst()->rangeExclusive(0, 3);
  return toPokemon.at(idx);
}

PokemonDBEntry* StarterPokemonDB::randomAnyStarter() const
{
  var32 idx = Random::inst()->rangeExclusive(0, store.size());
  return toPokemon.at(idx);
}

void StarterPokemonDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("starters");
  for (QJsonValue entry : jsonData.array())
    store.append(entry.toString());
  once = true;
}

void StarterPokemonDB::deepLink()
{
  static bool once = false;
  if (once) return;
  for (int i = 0; i < store.size(); ++i) {
    auto* mon = PokemonDB::inst()->getIndAt(store.at(i));
    toPokemon.append(mon);
#ifdef QT_DEBUG
    if (!mon) qCritical() << "Starter Pokemon:" << store.at(i) << "could not be deep linked.";
#endif
  }
  once = true;
}

void StarterPokemonDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void StarterPokemonDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<StarterPokemonDB>("PSE.DB.StarterPokemonDB", 1, 0, "StarterPokemonDB", "Can't instantiate in QML");
  once = true;
}

StarterPokemonDB::StarterPokemonDB()
{
  qmlRegister();
}
