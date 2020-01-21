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
#ifndef HOFRECORD_H
#define HOFRECORD_H

#include <QObject>
#include <QVector>
#include "../../../../common/types.h"
class SaveFile;
class HoFPokemon;

class HoFRecord : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QVector<HoFPokemon*> pokemon_ MEMBER pokemon NOTIFY pokemonChanged)

public:
  HoFRecord(SaveFile* saveFile = nullptr, var8 ind = 0);
  virtual ~HoFRecord();

signals:
  void pokemonChanged();

public slots:
  void load(SaveFile* saveFile = nullptr, var8 ind = 0);
  void save(SaveFile* saveFile, var8 ind);
  void reset();
  void randomize();

public:
  QVector<HoFPokemon*> pokemon;
};

#endif // HOFRECORD_H
