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
#include <QString>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
struct PokemonDBEntry;

/**
 * @brief One Pokemon entry within a Hall of Fame record: species, level, name.
 *
 * A minimal snapshot (the HoF only stores these three things per mon). One slot of
 * an HoFRecord.
 *
 * @see HoFRecord (the container of up to six of these), HallOfFame.
 */
class SAVEFILE_AUTOPORT HoFPokemon : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int species MEMBER species NOTIFY speciesChanged) ///< Species id.
  Q_PROPERTY(int level MEMBER level NOTIFY levelChanged)       ///< Level at the time.
  Q_PROPERTY(QString name MEMBER name NOTIFY nameChanged)      ///< Nickname recorded.

public:
  /// @param recordOffset HoF record base; @param ind slot within the record.
  HoFPokemon(SaveFile* saveFile = nullptr, var16 recordOffset = 0, var16 ind = 0);
  virtual ~HoFPokemon();

  void load(SaveFile* saveFile = nullptr, var16 recordOffset = 0, var16 ind = 0); ///< Expand from the save.
  void save(SaveFile* saveFile, var16 recordOffset, var16 ind);                   ///< Flatten to the save.

  PokemonDBEntry* toSpecies(); ///< Resolve @ref species to its DB entry.

signals:
  void speciesChanged();
  void levelChanged();
  void nameChanged();

public slots:
  void reset();     ///< Blank this entry.
  void randomize(); ///< Randomize this entry.

public:
  int species;   ///< @see species property.
  int level;     ///< @see level property.
  QString name;  ///< @see name property.
};
