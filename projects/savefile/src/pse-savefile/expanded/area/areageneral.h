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
#include "../../savefile_autoport.h"

class SaveFile;

/**
 * @brief Screen-contrast / flash levels, QML-visible.
 *
 * The valid contrast steps (Normal..SolidBlack) plus the intermediate "glitch"
 * values that fall between them -- exposed so the UI can present the real options
 * and label the glitchy in-between ones.
 */
struct SAVEFILE_AUTOPORT ContrastIds : public QObject
{
  Q_OBJECT
  Q_ENUMS(ContrastIds_)

public:
  enum ContrastIds_ : int
  {
    Normal = 0,               ///< Full brightness.
    Darken1 = 3,              ///< One step darker.
    Darken2_NeedsFlash = 6,   ///< Two steps darker (needs Flash).
    Darken3_SolidBlack = 9,   ///< Fully black.

    Glitch_1A = 1,            ///< Glitchy in-between value.
    Glitch_1B = 2,            ///< Glitchy in-between value.
    Glitch_2A = 4,            ///< Glitchy in-between value.
    Glitch_2B = 5,            ///< Glitchy in-between value.
    Glitch_3A = 7,            ///< Glitchy in-between value.
    Glitch_3B = 8             ///< Glitchy in-between value.
  };
};

class SAVEFILE_AUTOPORT MapDBEntry;

/**
 * @brief Miscellaneous per-area flags: contrast, letter delay, playtime counting.
 *
 * The smallest area child. @ref countPlaytime is the one field QML traverses the
 * area tree to reach (which is why area.h keeps AreaGeneral as a full include).
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see Area, ContrastIds.
 */
class SAVEFILE_AUTOPORT AreaGeneral : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int contrast MEMBER contrast NOTIFY contrastChanged)               ///< Screen contrast (see ContrastIds).
  Q_PROPERTY(bool noLetterDelay MEMBER noLetterDelay NOTIFY noLetterDelayChanged) ///< Disable text letter-by-letter delay.
  Q_PROPERTY(bool countPlaytime MEMBER countPlaytime NOTIFY countPlaytimeChanged) ///< Whether playtime is currently counting.

public:
  AreaGeneral(SaveFile* saveFile = nullptr);
  virtual ~AreaGeneral();

  void load(SaveFile* saveFile = nullptr); ///< Expand these flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten these flags to the save.

signals:
  void contrastChanged();
  void noLetterDelayChanged();
  void countPlaytimeChanged();

public slots:
  void reset();              ///< Blank the flags.
  void randomize();          ///< Randomize the flags.
  void setTo(MapDBEntry* map); ///< Set from a chosen map's defaults.

public:
  int contrast;        ///< @see contrast property.
  bool noLetterDelay;  ///< @see noLetterDelay property.
  bool countPlaytime;  ///< @see countPlaytime property.
};
