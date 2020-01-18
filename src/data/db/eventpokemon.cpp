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

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValueRef>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./eventpokemon.h"
#include "./gamedata.h"
#include "./pokemon.h"

EventPokemonDBEntry::EventPokemonDBEntry() {}
EventPokemonDBEntry::EventPokemonDBEntry(QJsonValue& data) {
  // Set simple properties
  title = data["title"].toString();
  desc = data["desc"].toString();
  pokemon = data["pokemon"].toString();
  region = data["region"].toString();

  // Set simple optional proeprties
  if(data["otID"].isString())
    otId = data["otID"].toString().toInt(nullptr, 16);

  if(data["level"].isDouble())
    level = data["level"].toDouble();

  // Set Moves
  auto movesArr = data["moves"].toArray();
  for(QJsonValue moveEntry : movesArr)
    moves.append(moveEntry.toString());

  // Set OT Name
  // Can be array or string
  if(data["otName"].isArray())
  {
    auto otNameArr = data["otName"].toArray();

    for(auto otNameEntry : otNameArr)
      otName.append(otNameEntry.toString());
  }
  else
    otName.append(data["otName"].toString());

  // Set DV
  if(data["dv"].isString())
  {
    // DV Can be
    // "max"             All DV Values of 15
    // ":##:##:##:##"    Specific DV Values
    //                   Atk, Def, Spd, Sp
    auto dvVal = data["dv"].toString();

    if(dvVal == "max") {
      dv.append(15);
      dv.append(15);
      dv.append(15);
      dv.append(15);
    }
    else
    {
      auto dvValParts = dvVal.split(":", QString::SkipEmptyParts);
      dv.append(dvValParts[0].toInt(nullptr, 10));
      dv.append(dvValParts[1].toInt(nullptr, 10));
      dv.append(dvValParts[2].toInt(nullptr, 10));
      dv.append(dvValParts[3].toInt(nullptr, 10));
    }
  }
}

void EventPokemonDBEntry::deepLink()
{
  toPokemon = PokemonDB::ind.value(pokemon, nullptr);

#ifdef QT_DEBUG
    if(toPokemon == nullptr)
      qCritical() << "Event Pokemon: " << pokemon << ", could not be deep linked." ;
#endif

    if(toPokemon != nullptr)
      toPokemon->toEventMons.append(this);
}

void EventPokemonDB::load()
{
  // Grab Event Pokemon Data
  auto eventPokemonData = GameData::json("eventPokemon");

  // Go through each event Pokemon
  for(QJsonValue eventPokemonEntry : eventPokemonData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new EventPokemonDBEntry(eventPokemonEntry);

    // Add to array
    store.append(entry);
  }

  delete eventPokemonData;
}

void EventPokemonDB::deepLink()
{
  for(auto entry : store)
  {
    entry->deepLink();
  }
}
