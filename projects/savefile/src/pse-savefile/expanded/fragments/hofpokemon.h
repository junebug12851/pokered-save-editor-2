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
#ifndef HOFPOKEMON_H
#define HOFPOKEMON_H

#include <QObject>
#include <QString>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
struct PokemonDBEntry;

class SAVEFILE_AUTOPORT HoFPokemon : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int species MEMBER species NOTIFY speciesChanged)
  Q_PROPERTY(int level MEMBER level NOTIFY levelChanged)
  Q_PROPERTY(QString name MEMBER name NOTIFY nameChanged)

public:
  HoFPokemon(SaveFile* saveFile = nullptr, var16 recordOffset = 0, var16 ind = 0);
  virtual ~HoFPokemon();

  void load(SaveFile* saveFile = nullptr, var16 recordOffset = 0, var16 ind = 0);
  void save(SaveFile* saveFile, var16 recordOffset, var16 ind);

  PokemonDBEntry* toSpecies();

signals:
  void speciesChanged();
  void levelChanged();
  void nameChanged();

public slots:
  void reset();
  void randomize();

public:
  int species;
  int level;
  QString name;
};

#endif // HOFPOKEMON_H
