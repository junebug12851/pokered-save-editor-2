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
class PlayerBasics;

// 2 Sets of 6 Pokemon Boxes
constexpr var8 maxPokemonStorageSets = 2;

class Storage : public QObject
{
  Q_OBJECT

  Q_PROPERTY(ItemStorageBox* items_ MEMBER items NOTIFY itemsChanged)
  Q_PROPERTY(var8 curBox_ MEMBER curBox NOTIFY curBoxChanged)
  Q_PROPERTY(bool boxesFormatted_ MEMBER boxesFormatted NOTIFY boxesFormattedChanged)

public:
  Storage(SaveFile* saveFile = nullptr);
  virtual ~Storage();

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
  PokemonStorageSet* pokemon[maxPokemonStorageSets];
  var8 curBox;
  bool boxesFormatted = false;
};

#endif // STORAGE_H
