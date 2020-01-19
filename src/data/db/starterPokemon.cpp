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

#include <QVector>
#include <QJsonArray>
#include <QtMath>
#include <QRandomGenerator>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./starterPokemon.h"
#include "./gamedata.h"
#include "./pokemon.h"

void StarterPokemonDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::json("starters");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData->array())
  {
    // Add to array
    store.append(jsonEntry.toString());
  }

  delete jsonData;
}

void StarterPokemonDB::deepLink()
{
  for(var8 i = 0; i < store.size(); i++)
  {
    auto entry = store.at(i); // Move Name

    // Deep link to tm number
    toPokemon.append(PokemonDB::ind.value(entry, nullptr));

#ifdef QT_DEBUG
    if(toPokemon.at(i) == nullptr)
      qCritical() << "Starter Pokemon: " << entry << ", could not be deep linked." ;
#endif
  }
}

PokemonDBEntry* StarterPokemonDB::random3Starter()
{
  // First 3 in list are in-game starters
  var32 ind = QRandomGenerator::global()->bounded(0, 3);
  return toPokemon.at(ind);
}

PokemonDBEntry* StarterPokemonDB::randomAnyStarter()
{
  // List as a whole are all potential starters
  var32 ind = QRandomGenerator::global()->bounded(0, store.size());
  return toPokemon.at(ind);
}

QVector<QString> StarterPokemonDB::store = QVector<QString>();
QVector<PokemonDBEntry*> StarterPokemonDB::toPokemon =
    QVector<PokemonDBEntry*>();
