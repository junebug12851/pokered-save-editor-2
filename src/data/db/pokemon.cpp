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
#include "pokemon.h"
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>
#include "./gamedata.h"

#include "./items.h"
#include "./moves.h"

#ifdef QT_DEBUG
#include <QtDebug>
#endif

EvolutionEntry::EvolutionEntry(QJsonValue& data)
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

  toDeEvolution = nullptr;
  toEvolution = nullptr;
  toItem = nullptr;
}

void EvolutionEntry::deepLink(PokemonEntry* deEvolution)
{
  // Link toEvolution first and ensure it's correct
  // otherwise it'll crrash and I want a clear error message why if it does
  toEvolution = Pokemon::ind->value(toName, nullptr);

  // Link to Item if present
  if(item != "")
    toItem = Items::ind->value(item, nullptr);

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
}

PokemonMoveEntry::PokemonMoveEntry(QJsonValue& data)
{
  // Set simple properties
  level = data["level"].toDouble();
  move = data["move"].toString();

  toMove = nullptr;
}

void PokemonMoveEntry::deepLink()
{
  toMove = Moves::ind->value(move, nullptr);

#ifdef QT_DEBUG
    if(toMove == nullptr)
      qCritical() << "Pokemon Move Entry: " << move << ", could not be deep linked." ;
#endif
}

PokemonEntry::PokemonEntry()
{
  name = "";
  glitch = false;
  type1 = "";
  type2 = "";

  moves = new QVector<PokemonMoveEntry*>();
  initial = new QVector<QString>();
  tmHm = new QVector<var8>;
  evolution = new QVector<EvolutionEntry*>;

  toType1 = nullptr;
  toType2 = nullptr;
  toDeEvolution = nullptr;
  toInitial = new QVector<MoveEntry*>();
  toTmHmMove = new QVector<MoveEntry*>();
  toTmHmItem = new QVector<ItemEntry*>();
}

void PokemonEntry::deepLink()
{
  // Set type 1 & 2 if present
  if(type1 != "")
    toType1 = Types::ind->value(type1, nullptr);
  if(type2 != "")
    toType2 = Types::ind->value(type2, nullptr);

#ifdef QT_DEBUG
    if((type1 != "") && type1 == nullptr)
      qCritical() << "Pokemon Type 1: " << type1 << ", could not be deep linked." ;
    if((type2 != "") && type2 == nullptr)
      qCritical() << "Pokemon Type 2: " << type2 << ", could not be deep linked." ;
#endif

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
    auto link = Moves::ind->value(initMove, nullptr);
    toInitial->append(link);

#ifdef QT_DEBUG
    if(link == nullptr)
      qCritical() << "Pokemon Initial Move: " << initMove << ", could not be deep linked." ;
#endif
  }

  // Deep-Link tm/hm
  for(auto tmHmMove : *tmHm)
  {
    auto moveLink = Moves::ind->value("tm" + QString::number(tmHmMove), nullptr);
    auto itemLink = Items::ind->value("tm" + QString::number(tmHmMove), nullptr);

    toTmHmMove->append(moveLink);
    toTmHmItem->append(itemLink);

#ifdef QT_DEBUG
    if(moveLink == nullptr)
      qCritical() << "Pokemon TM/HM Move: " << tmHmMove << ", could not be deep linked." ;
    if(itemLink == nullptr)
      qCritical() << "Pokemon TM/HM Item: " << tmHmMove << ", could not be deep linked." ;
#endif
  }
}

void Pokemon::load()
{
  // Grab Pokemon Data
  auto pokemonData = GameData::json("pokemon");

  // Go through each Pokemon
  for(QJsonValue pokemonEntry : pokemonData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new PokemonEntry();

    // Set simple properties
    entry->name = pokemonEntry["name"].toString();
    entry->ind = pokemonEntry["ind"].toDouble();
    entry->readable = pokemonEntry["readable"].toString();

    // Set simple optional properties
    if(pokemonEntry["pokedex"].isDouble())
      entry->pokedex = pokemonEntry["pokedex"].toDouble();

    if(pokemonEntry["growthRate"].isDouble())
      entry->growthRate = pokemonEntry["growthRate"].toDouble();

    if(pokemonEntry["baseHp"].isDouble())
      entry->baseHp = pokemonEntry["baseHp"].toDouble();

    if(pokemonEntry["baseAttack"].isDouble())
      entry->baseAttack = pokemonEntry["baseAttack"].toDouble();

    if(pokemonEntry["baseDefense"].isDouble())
      entry->baseDefense = pokemonEntry["baseDefense"].toDouble();

    if(pokemonEntry["baseSpeed"].isDouble())
      entry->baseSpeed = pokemonEntry["baseSpeed"].toDouble();

    if(pokemonEntry["baseSpecial"].isDouble())
      entry->baseSpecial = pokemonEntry["baseSpecial"].toDouble();

    if(pokemonEntry["baseExpYield"].isDouble())
      entry->baseExpYield = pokemonEntry["baseExpYield"].toDouble();

    if(pokemonEntry["type1"].isString())
      entry->type1 = pokemonEntry["type1"].toString();

    if(pokemonEntry["type2"].isString())
      entry->type2 = pokemonEntry["type2"].toString();

    if(pokemonEntry["catchRate"].isDouble())
      entry->catchRate = pokemonEntry["catchRate"].toDouble();

    if(pokemonEntry["glitch"].isDouble())
      entry->glitch = pokemonEntry["glitch"].toBool();

    // Set Moves
    if(pokemonEntry["moves"].isArray())
    {
      for(QJsonValue moveEntryz : pokemonEntry["moves"].toArray())
      {
        entry->moves->append(new PokemonMoveEntry(moveEntryz));
      }
    }

    // Set Initial
    if(pokemonEntry["initial"].isArray())
    {
      for(QJsonValue initialEntry : pokemonEntry["initial"].toArray())
      {
        entry->initial->append(initialEntry.toString());
      }
    }

    // Set TM HM
    if(pokemonEntry["tmHm"].isArray())
    {
      for(QJsonValue tmHmEntry : pokemonEntry["tmHm"].toArray())
      {
        entry->tmHm->append(tmHmEntry.toDouble());
      }
    }

    // Set Evolution
    // Is often next Pokemon up
    // Can be an Eevee Array for Eevee's multiple evolutions
    if(pokemonEntry["evolution"].isArray())
    {
      for(QJsonValue evolutionEntry : pokemonEntry["evolution"].toArray())
      {
        entry->evolution->append(new EvolutionEntry(evolutionEntry));
      }
    }
    else if(pokemonEntry["evolution"].isObject())
    {
      // Kinda weird this has to be 2 steps
      auto tmp = pokemonEntry["evolution"];
      entry->evolution->append(new EvolutionEntry(tmp));
    }

    // Add to array
    pokemon->append(entry);
  }
}

void Pokemon::index()
{
  for(auto entry : *pokemon)
  {
    // Index name and index
    ind->insert(entry->name, entry);
    ind->insert(QString::number(entry->ind), entry);
    ind->insert(entry->readable, entry);

    // If it's a valid Pokemon then index the dex number as well
    if(entry->pokedex)
      ind->insert("dex" + QString::number(*entry->pokedex), entry);
  }
}

void Pokemon::deepLink()
{
  // Deep link the Pokemon data structure tree, lots of traversing and stuff
  for(auto entry : *pokemon)
  {
    entry->deepLink();
  }
}

QVector<PokemonEntry*>* Pokemon::pokemon = new QVector<PokemonEntry*>();
QHash<QString, PokemonEntry*>* Pokemon::ind =
    new QHash<QString, PokemonEntry*>();
