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
#ifndef POKEMONSTORAGEBOX_H
#define POKEMONSTORAGEBOX_H

#include <QObject>
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class PokemonBox;
class PlayerBasics;

// Maximum pokemon that can fit into a box
constexpr var8 boxMaxPokemon = 20;

// Holds contents of a single Pokemon storage box
class SAVEFILE_AUTOPORT PokemonStorageBox : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int pokemonCount READ pokemonCount NOTIFY pokemonChanged)
  Q_PROPERTY(int isFull READ isFull NOTIFY pokemonChanged)
  Q_PROPERTY(int pokemonMax READ pokemonMax CONSTANT)

public:
  PokemonStorageBox(int maxSize = boxMaxPokemon, SaveFile* saveFile = nullptr, var16 boxOffset = 0);
  virtual ~PokemonStorageBox();

  virtual void load(SaveFile* saveFile = nullptr, var16 boxOffset = 0);
  virtual void save(SaveFile* saveFile, var16 boxOffset = 0);

  int pokemonCount();
  int pokemonMax();
  bool isFull();

  Q_INVOKABLE PokemonBox* pokemonAt(int ind);

signals:
  // When moving an item away from the box to another box, allow the model to
  // perform cleanup actions
  void beforePokemonRelocate(PokemonBox* item);

  void pokemonChanged();
  void pokemonMoveChange(int from, int to);
  void pokemonRemoveChange(int ind);
  void pokemonInsertChange();
  void pokemonResetChange();

public slots:
  void reset();
  virtual void randomize(PlayerBasics* basics);

  bool pokemonMove(int from, int to);
  void pokemonRemove(int ind);
  virtual void pokemonNew();

  bool relocateAll(PokemonStorageBox* dst);
  virtual bool relocateOne(PokemonStorageBox* dst, int ind);

public:
  bool isParty = false;
  QVector<PokemonBox*> pokemon;
  int maxSize = 0;
  SaveFile* file;
};

#endif // POKEMONSTORAGEBOX_H
