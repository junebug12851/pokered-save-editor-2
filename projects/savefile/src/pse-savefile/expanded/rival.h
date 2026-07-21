/*
  * Copyright 2020 Fairy Fox
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
#include "../savefile_autoport.h"

class SaveFile;

/**
 * @brief The rival: their name and chosen starter.
 *
 * A tiny top-level region. @ref starter selects which team the rival battles you
 * with (see the field note -- only the three real starters are valid). Standard
 * expanded-node convention (see SaveFileExpanded).
 *
 * @see SaveFileExpanded, PlayerBasics (the player's mirror).
 */
class SAVEFILE_AUTOPORT Rival : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString name MEMBER name NOTIFY nameChanged)       ///< Rival's name.
  Q_PROPERTY(int starter MEMBER starter NOTIFY starterChanged)  ///< Rival's starter (drives his team; see field note).

public:
  Rival(SaveFile* saveFile = nullptr);
  virtual ~Rival();

  void load(SaveFile* saveFile = nullptr); ///< Expand the rival data from the save.
  void save(SaveFile* saveFile);           ///< Flatten the rival data to the save.

signals:
  void nameChanged();
  void starterChanged();

public slots:
  void reset();     ///< Blank the rival.
  void randomize(); ///< Randomize the rival's name/starter.

public:
  // Rival's Name and Starter Pokemon
  // This essentially selects his team that he uses to battle you, it goes
  // by internal Pokemon index and only 3 options are valid, Charmander,
  // Bulbasaur, and Squirtle. I have no idea what will happen if you put a
  // different value in here.
  QString name;  ///< @see name property.
  int starter;   ///< @see starter property (valid: Charmander/Bulbasaur/Squirtle index).
};
