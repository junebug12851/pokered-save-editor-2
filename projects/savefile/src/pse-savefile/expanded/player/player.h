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
#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include "../../savefile_autoport.h"

class PlayerBasics;
class PlayerPokedex;
class PlayerPokemon;
class ItemStorageBox;

class SaveFile;

class SAVEFILE_AUTOPORT Player : public QObject
{
  Q_OBJECT

  Q_PROPERTY(PlayerBasics* basics MEMBER basics NOTIFY basicsChanged)
  Q_PROPERTY(ItemStorageBox* items MEMBER items NOTIFY itemsChanged)
  Q_PROPERTY(PlayerPokedex* pokedex MEMBER pokedex NOTIFY pokedexChanged)
  Q_PROPERTY(PlayerPokemon* pokemon MEMBER pokemon NOTIFY pokemonChanged)

public:
  Player(SaveFile* saveFile = nullptr);
  virtual ~Player();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void basicsChanged();
  void itemsChanged();
  void pokedexChanged();
  void pokemonChanged();

public slots:
  void reset();
  void randomize();

public:
  PlayerBasics* basics = nullptr;
  ItemStorageBox* items = nullptr;
  PlayerPokedex* pokedex = nullptr;
  PlayerPokemon* pokemon = nullptr;
};

#endif // PLAYER_H
