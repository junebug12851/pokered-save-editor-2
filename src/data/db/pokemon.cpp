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
#include <QJsonObject>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./pokemon.h"
#include "./gamedata.h"
#include "./items.h"
#include "./moves.h"
#include "./types.h"

PokemonDBEntryEvolution::PokemonDBEntryEvolution() {}
PokemonDBEntryEvolution::PokemonDBEntryEvolution(QJsonValue& data, PokemonDBEntry* parent) :
  parent(parent)
{
  // Set simple properties
  toName = data["toName"].toString();

  // Set simple optional properties
  if(data["level"].isDouble())
    level = data["level"].toDouble();

  if(data["trade"].isBool())
    trade = data["trade"].toBool();
  else
    trade = false;

  if(data["item"].isString())
    item = data["item"].toString();
  else
    item = "";
}

void PokemonDBEntryEvolution::deepLink(PokemonDBEntry* deEvolution)
{
  // Link toEvolution first and ensure it's correct
  // otherwise it'll crrash and I want a clear error message why if it does
  toEvolution = PokemonDB::ind.value(toName, nullptr);

  // Link to Item if present
  if(item != "")
    toItem = ItemsDB::ind.value(item, nullptr);

#ifdef QT_DEBUG
    if(toEvolution == nullptr)
      qCritical() << "Evolution Entry: " << toName << ", could not be deep linked." ;
    if(deEvolution == nullptr)
      qCritical() << "Evolution Entry: " << toName << ", null deEvolution provided" ;
    if((item != "") && toItem == nullptr)
      qCritical() << "Evolution Entry: " << item << ", item could not be deep linked" ;
#endif

    // Link to deEvolution in it's evolution and here
    toEvolution->toDeEvolution = deEvolution;
    toDeEvolution = deEvolution;

    if(toItem != nullptr)
      toItem->toEvolvePokemon.append(this);
}

PokemonDBEntryMove::PokemonDBEntryMove() {}
PokemonDBEntryMove::PokemonDBEntryMove(QJsonValue& data, PokemonDBEntry* parent) :
  parent(parent)
{
  // Set simple properties
  level = data["level"].toDouble();
  move = data["move"].toString();
}

void PokemonDBEntryMove::deepLink()
{
  toMove = MovesDB::ind.value(move, nullptr);

#ifdef QT_DEBUG
    if(toMove == nullptr)
      qCritical() << "Pokemon Move Entry: " << move << ", could not be deep linked." ;
#endif

    if(toMove != nullptr)
      toMove->toPokemonLearned.append(this);
}

PokemonDBEntry::PokemonDBEntry() {}
PokemonDBEntry::PokemonDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  readable = data["readable"].toString();

  // Set simple optional properties
  if(data["pokedex"].isDouble())
    pokedex = data["pokedex"].toDouble();

  if(data["growthRate"].isDouble())
    growthRate = data["growthRate"].toDouble();

  if(data["baseHp"].isDouble())
    baseHp = data["baseHp"].toDouble();

  if(data["baseAttack"].isDouble())
    baseAttack = data["baseAttack"].toDouble();

  if(data["baseDefense"].isDouble())
    baseDefense = data["baseDefense"].toDouble();

  if(data["baseSpeed"].isDouble())
    baseSpeed = data["baseSpeed"].toDouble();

  if(data["baseSpecial"].isDouble())
    baseSpecial = data["baseSpecial"].toDouble();

  if(data["baseExpYield"].isDouble())
    baseExpYield = data["baseExpYield"].toDouble();

  if(data["type1"].isString())
    type1 = data["type1"].toString();

  if(data["type2"].isString())
    type2 = data["type2"].toString();

  if(data["catchRate"].isDouble())
    catchRate = data["catchRate"].toDouble();

  if(data["glitch"].isDouble())
    glitch = data["glitch"].toBool();

  // Set Moves
  if(data["moves"].isArray())
  {
    for(QJsonValue moveEntryz : data["moves"].toArray())
    {
      moves->append(new PokemonDBEntryMove(moveEntryz, this));
    }
  }

  // Set Initial
  if(data["initial"].isArray())
  {
    for(QJsonValue initialEntry : data["initial"].toArray())
    {
      initial->append(initialEntry.toString());
    }
  }

  // Set TM HM
  if(data["tmHm"].isArray())
  {
    for(QJsonValue tmHmEntry : data["tmHm"].toArray())
    {
      tmHm->append(tmHmEntry.toDouble());
    }
  }

  // Set Evolution
  // Is often next Pokemon up
  // Can be an Eevee Array for Eevee's multiple evolutions
  if(data["evolution"].isArray())
  {
    for(QJsonValue evolutionEntry : data["evolution"].toArray())
    {
      evolution->append(new PokemonDBEntryEvolution(evolutionEntry, this));
    }
  }
  else if(data["evolution"].isObject())
  {
    // Kinda weird this has to be 2 steps
    auto tmp = data["evolution"];
    evolution->append(new PokemonDBEntryEvolution(tmp, this));
  }
}

void PokemonDBEntry::deepLink()
{
  // Set type 1 & 2 if present
  if(type1 != "")
    toType1 = TypesDB::ind.value(type1, nullptr);
  if(type2 != "")
    toType2 = TypesDB::ind.value(type2, nullptr);

#ifdef QT_DEBUG
    if((type1 != "") && type1 == nullptr)
      qCritical() << "Pokemon Type 1: " << type1 << ", could not be deep linked." ;
    if((type2 != "") && type2 == nullptr)
      qCritical() << "Pokemon Type 2: " << type2 << ", could not be deep linked." ;
#endif

    if(toType1 != nullptr)
      toType1->toPokemon.append(this);

    if(toType2 != nullptr)
      toType2->toPokemon.append(this);

  // De-Evolution is set by evolution entry
  for(auto evolEntry : *evolution)
  {
    // Initiate deep linking for all evolutions
    evolEntry->deepLink(this);
  }

  // Level Moves is set by Pokemon moves entry
  for(auto pokeMoveEntry : *moves)
  {
    // Initiate deep linking for all level moves
    pokeMoveEntry->deepLink();
  }

  // Deep-Link initial moves
  for(auto initMove : *initial)
  {
    auto link = MovesDB::ind.value(initMove, nullptr);
    toInitial.append(link);

#ifdef QT_DEBUG
    if(link == nullptr)
      qCritical() << "Pokemon Initial Move: " << initMove << ", could not be deep linked." ;
#endif

    if(link != nullptr)
      link->toPokemonInitial.append(this);
  }

  // Deep-Link tm/hm
  for(auto tmHmMove : *tmHm)
  {
    auto moveLink = MovesDB::ind.value("tm" + QString::number(tmHmMove), nullptr);
    auto itemLink = ItemsDB::ind.value("tm" + QString::number(tmHmMove), nullptr);

    toTmHmMove.append(moveLink);
    toTmHmItem.append(itemLink);

#ifdef QT_DEBUG
    if(moveLink == nullptr)
      qCritical() << "Pokemon TM/HM Move: " << tmHmMove << ", could not be deep linked." ;
    if(itemLink == nullptr)
      qCritical() << "Pokemon TM/HM Item: " << tmHmMove << ", could not be deep linked." ;
#endif

    if(moveLink != nullptr)
      moveLink->toPokemonTmHm.append(this);

    if(itemLink != nullptr)
      itemLink->toTeachPokemon.append(this);
  }
}

void PokemonDB::load()
{
  // Grab Pokemon Data
  auto pokemonData = GameData::json("pokemon");

  // Go through each Pokemon
  for(QJsonValue pokemonEntry : pokemonData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new PokemonDBEntry(pokemonEntry);

    // Add to array
    store.append(entry);
  }

  delete pokemonData;
}

void PokemonDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
    ind.insert(entry->readable, entry);

    // If it's a valid Pokemon then index the dex number as well
    if(entry->pokedex)
      ind.insert("dex" + QString::number(*entry->pokedex), entry);
  }
}

void PokemonDB::deepLink()
{
  // Deep link the Pokemon data structure tree, lots of traversing and stuff
  for(auto entry : store)
  {
    entry->deepLink();
  }
}

QVector<PokemonDBEntry*> PokemonDB::store = QVector<PokemonDBEntry*>();
QHash<QString, PokemonDBEntry*> PokemonDB::ind =
    QHash<QString, PokemonDBEntry*>();
