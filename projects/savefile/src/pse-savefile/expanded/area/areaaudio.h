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
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class MapDBEntry;

/**
 * @brief The current map's audio: which track plays, and a couple of music flags.
 *
 * Holds the @ref musicID / @ref musicBank that select the playing track, plus
 * @ref noAudioFadeout and @ref preventMusicChange behaviour flags. Standard
 * expanded-node convention (see SaveFileExpanded); setTo() pulls the right music
 * for a chosen map.
 *
 * @see Area (container), MapDBEntry.
 */
class SAVEFILE_AUTOPORT AreaAudio : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int musicID MEMBER musicID NOTIFY musicIDChanged)                       ///< Playing track id.
  Q_PROPERTY(int musicBank MEMBER musicBank NOTIFY musicBankChanged)                 ///< Bank the track lives in.
  Q_PROPERTY(bool noAudioFadeout MEMBER noAudioFadeout NOTIFY noAudioFadeoutChanged) ///< Skip the audio fade-out.
  Q_PROPERTY(bool preventMusicChange MEMBER preventMusicChange NOTIFY preventMusicChangeChanged) ///< Lock the current music.

public:
  AreaAudio(SaveFile* saveFile = nullptr);
  virtual ~AreaAudio();

  void load(SaveFile* saveFile = nullptr); ///< Expand audio settings from the save.
  void save(SaveFile* saveFile);           ///< Flatten audio settings to the save.

signals:
  void musicIDChanged();
  void musicBankChanged();
  void noAudioFadeoutChanged();
  void preventMusicChangeChanged();

public slots:
  void reset();              ///< Blank audio settings.
  void randomize();          ///< Randomize the track.
  void setTo(MapDBEntry* map); ///< Set music to @p map's default.

public:
  int musicID;             ///< @see musicID property.
  int musicBank;           ///< @see musicBank property.
  bool noAudioFadeout;     ///< @see noAudioFadeout property.
  bool preventMusicChange; ///< @see preventMusicChange property.
};
