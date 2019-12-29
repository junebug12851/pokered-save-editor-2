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
}

PokemonMoveEntry::PokemonMoveEntry(QJsonValue& data)
{
  // Set simple properties
  level = data["level"].toDouble();
  move = data["move"].toString();
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

QVector<PokemonEntry*>* Pokemon::pokemon = new QVector<PokemonEntry*>();
QHash<QString, PokemonEntry*>* Pokemon::ind =
    new QHash<QString, PokemonEntry*>();
