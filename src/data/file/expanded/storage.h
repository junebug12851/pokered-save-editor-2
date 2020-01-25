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
#ifndef STORAGE_H
#define STORAGE_H

#include <QObject>
#include "../../../common/types.h"
class SaveFile;

class ItemStorageBox;
class PokemonStorageSet;
class PokemonStorageBox;
class PlayerBasics;

// 2 Sets of 6 Pokemon Boxes
constexpr var8 maxPokemonStorageSets = 2;
constexpr var8 maxPokemonBoxes = maxPokemonStorageSets * 6;

class Storage : public QObject
{
  Q_OBJECT

  Q_PROPERTY(ItemStorageBox* items MEMBER items NOTIFY itemsChanged)
  Q_PROPERTY(int curBox MEMBER curBox NOTIFY curBoxChanged)
  Q_PROPERTY(bool boxesFormatted MEMBER boxesFormatted NOTIFY boxesFormattedChanged)

public:
  Storage(SaveFile* saveFile = nullptr);
  virtual ~Storage();

  // Sets are a fixed size and cannot be moved, created, modified, or destroyed
  Q_INVOKABLE int setCount();
  Q_INVOKABLE PokemonStorageSet* setAt(int ind);

  // Ignores the sets and returns the box with given box number
  Q_INVOKABLE int boxCount();
  Q_INVOKABLE PokemonStorageBox* boxAt(int ind);

signals:
  void itemsChanged();
  void curBoxChanged();
  void boxesFormattedChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize(PlayerBasics* basics);

public:
  ItemStorageBox* items = nullptr;
  int curBox;
  bool boxesFormatted = false;

  // Because this is a C++ array, it can't be a Q_PROPERTY and since the array
  // contents never change and have no properties of themselves there's no need
  // for a signal either
  PokemonStorageSet* pokemon[maxPokemonStorageSets];
};

#endif // STORAGE_H
