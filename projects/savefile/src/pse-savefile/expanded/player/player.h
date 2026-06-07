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
#include <QObject>
#include "../../savefile_autoport.h"

#include "./playerbasics.h"
#include "./playerpokedex.h"
#include "./playerpokemon.h"
#include "../fragments/itemstoragebox.h"

class PlayerBasics;
class PlayerPokedex;
class PlayerPokemon;
class ItemStorageBox;

class SaveFile;

/**
 * @brief The trainer: their basics, bag, pokedex, and party.
 *
 * Groups the four player-owned regions under one node of the expanded tree.
 * Follows the standard expanded-node convention (load/save/reset/randomize) --
 * see SaveFileExpanded for the full explanation.
 *
 * @see SaveFileExpanded, PlayerBasics, PlayerPokedex, PlayerPokemon, ItemStorageBox
 */
class SAVEFILE_AUTOPORT Player : public QObject
{
  Q_OBJECT

  Q_PROPERTY(PlayerBasics* basics MEMBER basics NOTIFY basicsChanged) ///< Name, ID, money, coins, badges, starter.
  Q_PROPERTY(ItemStorageBox* items MEMBER items NOTIFY itemsChanged)  ///< The trainer's bag.
  Q_PROPERTY(PlayerPokedex* pokedex MEMBER pokedex NOTIFY pokedexChanged) ///< Seen/owned dex flags.
  Q_PROPERTY(PlayerPokemon* pokemon MEMBER pokemon NOTIFY pokemonChanged) ///< The party (a specialized storage box).

public:
  Player(SaveFile* saveFile = nullptr);
  virtual ~Player();

  void load(SaveFile* saveFile = nullptr); ///< Expand the player regions from the save.
  void save(SaveFile* saveFile);           ///< Flatten the player regions back to the save.

signals:
  void basicsChanged();
  void itemsChanged();
  void pokedexChanged();
  void pokemonChanged();

public slots:
  void reset();     ///< Blank all player regions.
  void randomize(); ///< Randomize all player regions (constrained).

public:
  PlayerBasics* basics = nullptr;   ///< @see basics property.
  ItemStorageBox* items = nullptr;  ///< @see items property.
  PlayerPokedex* pokedex = nullptr; ///< @see pokedex property.
  PlayerPokemon* pokemon = nullptr; ///< @see pokemon property.
};
