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
#ifndef MAPDBENTRYSPRITEPOKEMON_H
#define MAPDBENTRYSPRITEPOKEMON_H

#include "./mapdbentrysprite.h"
#include "../db_autoport.h"

class PokemonDBEntry;
class MapsDB;

// A Pokemon that can be battled
struct DB_AUTOPORT MapDBEntrySpritePokemon : public MapDBEntrySprite
{
  Q_OBJECT
  Q_PROPERTY(QString getPokemon READ getPokemon CONSTANT)
  Q_PROPERTY(int getLevel READ getLevel CONSTANT)
  Q_PROPERTY(PokemonDBEntry* getToPokemon READ getToPokemon CONSTANT)

public:
  virtual SpriteType type() const;
  const QString getPokemon() const;
  int getLevel() const;
  const PokemonDBEntry* getToPokemon() const;

protected:
  MapDBEntrySpritePokemon(const QJsonValue& data, MapDBEntry* const parent);
  virtual void deepLink();
  virtual void qmlRegister() const;

  // Pokemon Details
  QString pokemon = "";
  int level = -1;

  PokemonDBEntry* toPokemon = nullptr;

  friend class MapDBEntry;
};

#endif // MAPDBENTRYSPRITEPOKEMON_H
