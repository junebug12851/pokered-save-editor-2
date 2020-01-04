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

EventPokemonDBEntry::EventPokemonDBEntry()
{
  otName = QVector<QString>();
  moves = QVector<QString>();
}

void EventPokemonDB::load()
{
  // Grab Event Pokemon Data
  auto eventPokemonData = GameData::json("eventPokemon");

  // Go through each event Pokemon
  for(QJsonValue eventPokemonEntry : eventPokemonData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new EventPokemonEntry();

    // Set simple properties
    entry->title = eventPokemonEntry["title"].toString();
    entry->desc = eventPokemonEntry["desc"].toString();
    entry->pokemon = eventPokemonEntry["pokemon"].toString();
    entry->region = eventPokemonEntry["region"].toString();

    // Set simple optional proeprties
    if(eventPokemonEntry["otID"].isString())
      entry->otId = eventPokemonEntry["otID"].toString().toInt(nullptr, 16);

    if(eventPokemonEntry["level"].isDouble())
      entry->level = eventPokemonEntry["level"].toDouble();

    // Set Moves
    auto movesArr = eventPokemonEntry["moves"].toArray();
    for(QJsonValue moveEntry : movesArr)
      entry->moves.append(moveEntry.toString());

    // Set OT Name
    // Can be array or string
    if(eventPokemonEntry["otName"].isArray())
    {
      auto otNameArr = eventPokemonEntry["otName"].toArray();

      for(auto otNameEntry : otNameArr)
        entry->otName.append(otNameEntry.toString());
    }
    else
      entry->otName.append(eventPokemonEntry["otName"].toString());

    // Set DV
    if(eventPokemonEntry["dv"].isString())
    {
      entry->dv = QVector<var8>();

      // DV Can be
      // "max"             All DV Values of 15
      // ":##:##:##:##"    Specific DV Values
      //                   Atk, Def, Spd, Sp
      auto dvVal = eventPokemonEntry["dv"].toString();
      if(dvVal == "max") {
        entry->dv.append(15);
        entry->dv.append(15);
        entry->dv.append(15);
        entry->dv.append(15);
      }
      else
      {
        auto dvValParts = dvVal.split(":", QString::SkipEmptyParts);
        entry->dv.append(dvValParts[0].toInt(nullptr, 10));
        entry->dv.append(dvValParts[1].toInt(nullptr, 10));
        entry->dv.append(dvValParts[2].toInt(nullptr, 10));
        entry->dv.append(dvValParts[3].toInt(nullptr, 10));
      }
    }

    // Add to array
    store.append(entry);
  }
}

void EventPokemonDB::deepLink()
{
  for(auto entry : store)
  {
    entry->toPokemon = Pokemon::ind->value(entry->pokemon, nullptr);

#ifdef QT_DEBUG
    if(entry->toPokemon == nullptr)
      qCritical() << "Event Pokemon: " << entry->pokemon << ", could not be deep linked." ;
#endif
  }
}

QVector<EventPokemonEntry*>* EventPokemon::eventPokemon =
    new QVector<EventPokemonEntry*>;
