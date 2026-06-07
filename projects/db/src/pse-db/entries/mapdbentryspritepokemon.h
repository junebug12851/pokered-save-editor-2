/*
  * Copyright 2020 Twilight
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
#pragma once
#include "./mapdbentrysprite.h"
#include "../db_autoport.h"

class PokemonDBEntry;
class MapsDB;

// A Pokemon that can be battled
/**
 * @brief A map sprite that is a static, battleable Pokemon (type POKEMON).
 *
 * Adds the @ref pokemon species and @ref level (species resolved to
 * @ref toPokemon in deepLink) to MapDBEntrySprite. type() returns POKEMON.
 * See db.md.
 *
 * @see MapDBEntrySprite (base), PokemonDBEntry.
 */
struct DB_AUTOPORT MapDBEntrySpritePokemon : public MapDBEntrySprite
{
  Q_OBJECT
  Q_PROPERTY(QString getPokemon READ getPokemon CONSTANT)  ///< Species name.
  Q_PROPERTY(int getLevel READ getLevel CONSTANT)          ///< Encounter level.
  Q_PROPERTY(PokemonDBEntry* getToPokemon READ getToPokemon CONSTANT) ///< Resolved species.

public:
  virtual SpriteType type() const;       ///< Returns POKEMON.
  const QString getPokemon() const;      ///< @see getPokemon property.
  int getLevel() const;                  ///< @see getLevel property.
  PokemonDBEntry* getToPokemon() const;  ///< @see getToPokemon property.

protected:
  MapDBEntrySpritePokemon(const QJsonValue& data, MapDBEntry* const parent); ///< Build from JSON under @p parent.
  virtual void deepLink();          ///< Resolve the species link.
  virtual void qmlRegister() const; ///< Register with QML.

  // Pokemon Details
  QString pokemon = ""; ///< Species name (read via getPokemon()).
  int level = -1;       ///< Encounter level.

  PokemonDBEntry* toPokemon = nullptr; ///< Resolved species (deepLink).

  friend class MapDBEntry;
};
