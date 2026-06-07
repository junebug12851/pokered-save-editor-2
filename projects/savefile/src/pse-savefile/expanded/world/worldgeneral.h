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
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

/**
 * @brief In-game Options menu settings (text speed, battle style, animations).
 *
 * A small sub-object of WorldGeneral mirroring the player's chosen options.
 */
class SAVEFILE_AUTOPORT Options : public QObject {
  Q_OBJECT

  Q_PROPERTY(int textSlowness MEMBER textSlowness NOTIFY textSlownessChanged)       ///< Text speed setting.
  Q_PROPERTY(bool battleStyleSet MEMBER battleStyleSet NOTIFY battleStyleSetChanged) ///< Battle style "Set" (vs "Shift").
  Q_PROPERTY(bool battleAnimOff MEMBER battleAnimOff NOTIFY battleAnimOffChanged)   ///< Battle animations off.

signals:
  void textSlownessChanged();
  void battleStyleSetChanged();
  void battleAnimOffChanged();

public:
  int textSlowness;    ///< @see textSlowness property.
  bool battleStyleSet; ///< @see battleStyleSet property.
  bool battleAnimOff;  ///< @see battleAnimOff property.
};

/**
 * @brief The text letter-printing delay flags (paired with the Options text speed).
 */
class SAVEFILE_AUTOPORT LetterDelay : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool normalDelay MEMBER normalDelay NOTIFY normalDelayChanged) ///< Normal per-letter delay.
  Q_PROPERTY(bool dontDelay MEMBER dontDelay NOTIFY dontDelayChanged)       ///< Skip the per-letter delay.

signals:
  void normalDelayChanged();
  void dontDelayChanged();

public:
  bool normalDelay; ///< @see normalDelay property.
  bool dontDelay;   ///< @see dontDelay property.
};

/**
 * @brief General world settings: last maps plus the Options / LetterDelay objects.
 *
 * Holds @ref lastBlackoutMap (where a blackout sends you) and @ref lastMap, and
 * nests the @ref options and @ref letterDelay sub-objects. Standard expanded-node
 * convention (see SaveFileExpanded).
 *
 * @see World, Options, LetterDelay.
 */
class SAVEFILE_AUTOPORT WorldGeneral : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int lastBlackoutMap MEMBER lastBlackoutMap NOTIFY lastBlackoutMapChanged) ///< Map a blackout returns you to.
  Q_PROPERTY(int lastMap MEMBER lastMap NOTIFY lastMapChanged)                          ///< Last map visited.
  Q_PROPERTY(Options* options MEMBER options NOTIFY optionsChanged)                     ///< In-game options.
  Q_PROPERTY(LetterDelay* letterDelay MEMBER letterDelay NOTIFY letterDelayChanged)     ///< Letter-delay flags.

public:
  WorldGeneral(SaveFile* saveFile = nullptr);
  virtual ~WorldGeneral();

  void load(SaveFile* saveFile = nullptr); ///< Expand general world settings from the save.
  void save(SaveFile* saveFile);           ///< Flatten general world settings to the save.

signals:
  void lastBlackoutMapChanged();
  void lastMapChanged();
  void optionsChanged();
  void letterDelayChanged();

public slots:
  void reset();     ///< Blank these settings.
  void randomize(); ///< Randomize these settings.

public:
  int lastBlackoutMap;       ///< @see lastBlackoutMap property.
  int lastMap;               ///< @see lastMap property.
  Options* options;          ///< @see options property.
  LetterDelay* letterDelay;  ///< @see letterDelay property.
};
