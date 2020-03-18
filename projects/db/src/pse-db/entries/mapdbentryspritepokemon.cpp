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

#include <QDebug>
#include <QJsonValue>
#include <QQmlEngine>
#include "mapdbentryspritepokemon.h"
#include "../pokemon.h"

MapDBEntrySpritePokemon::MapDBEntrySpritePokemon(const QJsonValue& data,
                                                 MapDBEntry* const parent) :
  MapDBEntrySprite(data, parent)
{
  pokemon = data["pokemon"].toString();
  level = data["level"].toDouble();
}

void MapDBEntrySpritePokemon::deepLink()
{
  MapDBEntrySprite::deepLink();
  toPokemon = PokemonDB::ind.value(pokemon);

#ifdef QT_DEBUG
  if(toPokemon == nullptr)
    qCritical() << "MapDBEntrySpritePokemon: Unable to deep link " + pokemon + " to pokemon";
#endif

  if(toPokemon != nullptr)
    toPokemon->toMapSpritePokemon = this;
}

void MapDBEntrySpritePokemon::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntrySpritePokemon>(
        "PSE.DB.MapDBEntrySpritePokemon", 1, 0, "MapDBEntrySpritePokemon", "Can't instantiate in QML");
  once = true;
}

const PokemonDBEntry* MapDBEntrySpritePokemon::getToPokemon() const
{
    return toPokemon;
}

int MapDBEntrySpritePokemon::getLevel() const
{
    return level;
}

const QString MapDBEntrySpritePokemon::getPokemon() const
{
    return pokemon;
}

MapDBEntrySpritePokemon::SpriteType MapDBEntrySpritePokemon::type() const
{
    return SpriteType::POKEMON;
}
