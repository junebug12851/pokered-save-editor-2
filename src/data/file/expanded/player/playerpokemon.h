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
#ifndef PLAYERPOKEMON_H
#define PLAYERPOKEMON_H

#include <QObject>
#include <QVector>
#include "../../../../common/types.h"
class SaveFile;
class PlayerBasics;
class PokemonParty;

constexpr var8 maxParty = 6;

class PlayerPokemon : public QObject
{
  Q_OBJECT

public:
  PlayerPokemon(SaveFile* saveFile = nullptr);
  virtual ~PlayerPokemon();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int partyCount();
  Q_INVOKABLE int partyMax();
  Q_INVOKABLE PokemonParty* partyAt(int ind);
  Q_INVOKABLE void partySwap(int from, int to);
  Q_INVOKABLE void partyRemove(int ind);
  Q_INVOKABLE void partyNew();

signals:
  void partyChanged();

public slots:
  void reset();
  void randomize(PlayerBasics* basics);

public:
  QVector<PokemonParty*> party;
};

#endif // PLAYERPOKEMON_H
