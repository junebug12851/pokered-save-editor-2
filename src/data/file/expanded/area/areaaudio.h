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
#ifndef AREAAUDIO_H
#define AREAAUDIO_H

#include <QObject>
#include "../../../../common/types.h"
class SaveFile;

class AreaAudio : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int musicID MEMBER musicID NOTIFY musicIDChanged)
  Q_PROPERTY(int musicBank MEMBER musicBank NOTIFY musicBankChanged)
  Q_PROPERTY(bool noAudioFadeout MEMBER noAudioFadeout NOTIFY noAudioFadeoutChanged)
  Q_PROPERTY(bool preventMusicChange MEMBER preventMusicChange NOTIFY preventMusicChangeChanged)

public:
  AreaAudio(SaveFile* saveFile = nullptr);
  virtual ~AreaAudio();

signals:
  void musicIDChanged();
  void musicBankChanged();
  void noAudioFadeoutChanged();
  void preventMusicChangeChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  int musicID;
  int musicBank;
  bool noAudioFadeout;
  bool preventMusicChange;
};

#endif // AREAAUDIO_H
