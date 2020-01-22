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

class PlayerBasics;
class PlayerItems;
class PlayerPokedex;
class PlayerPokemon;

class SaveFile;

class Player : public QObject
{
  Q_OBJECT

  Q_PROPERTY(PlayerBasics* basics_ MEMBER basics NOTIFY basicsChanged)
  Q_PROPERTY(PlayerItems* items_ MEMBER items NOTIFY itemsChanged)
  Q_PROPERTY(PlayerPokedex* pokedex_ MEMBER pokedex NOTIFY pokedexChanged)
  Q_PROPERTY(PlayerPokemon* pokemon_ MEMBER pokemon NOTIFY pokemonChanged)

public:
  Player(SaveFile* saveFile = nullptr);
  virtual ~Player();

signals:
  void basicsChanged();
  void itemsChanged();
  void pokedexChanged();
  void pokemonChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  PlayerBasics* basics = nullptr;
  PlayerItems* items = nullptr;
  PlayerPokedex* pokedex = nullptr;
  PlayerPokemon* pokemon = nullptr;
};

#endif // PLAYER_H
