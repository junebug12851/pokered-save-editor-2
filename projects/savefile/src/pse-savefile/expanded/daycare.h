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

#include <pse-common/types.h>
#include "../savefile_autoport.h"

// PlayerBasics is used as a slot parameter below; include the full type so its
// QMetaType resolves now that it is no longer Q_DECLARE_OPAQUE_POINTER'd.
#include "./player/playerbasics.h"
// PokemonBox is a Q_PROPERTY below — full include so QML can traverse it.
#include "./fragments/pokemonbox.h"

class SaveFile;
class PokemonBox;
class PlayerBasics;

class SAVEFILE_AUTOPORT Daycare : public QObject
{
  Q_OBJECT

  Q_PROPERTY(PokemonBox* pokemon MEMBER pokemon NOTIFY pokemonChanged)

public:
  Daycare(SaveFile* saveFile = nullptr);
  virtual ~Daycare();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void pokemonChanged();

public slots:
  void reset();
  void randomize(PlayerBasics* basics);

public:
  PokemonBox* pokemon = nullptr;
};
