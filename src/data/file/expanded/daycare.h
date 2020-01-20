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
#ifndef DAYCARE_H
#define DAYCARE_H

#include <QObject>

#include "../../../common/types.h"
class SaveFile;
class PokemonBox;
class PlayerBasics;

class Daycare : public QObject
{
  Q_OBJECT

  Q_PROPERTY(PokemonBox* pokemon_ MEMBER pokemon NOTIFY pokemonChanged)

public:
  Daycare(SaveFile* saveFile = nullptr);
  virtual ~Daycare();

signals:
  void pokemonChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize(PlayerBasics* basics);

public:
  PokemonBox* pokemon = nullptr;
};

#endif // DAYCARE_H
