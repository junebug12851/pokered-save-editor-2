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
#ifndef WORLDGENERAL_H
#define WORLDGENERAL_H

#include <QObject>
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

class SAVEFILE_AUTOPORT Options : public QObject {
  Q_OBJECT

  Q_PROPERTY(int textSlowness MEMBER textSlowness NOTIFY textSlownessChanged)
  Q_PROPERTY(bool battleStyleSet MEMBER battleStyleSet NOTIFY battleStyleSetChanged)
  Q_PROPERTY(bool battleAnimOff MEMBER battleAnimOff NOTIFY battleAnimOffChanged)

signals:
  void textSlownessChanged();
  void battleStyleSetChanged();
  void battleAnimOffChanged();

public:
  int textSlowness;
  bool battleStyleSet;
  bool battleAnimOff;
};

class SAVEFILE_AUTOPORT LetterDelay : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool normalDelay MEMBER normalDelay NOTIFY normalDelayChanged)
  Q_PROPERTY(bool dontDelay MEMBER dontDelay NOTIFY dontDelayChanged)

signals:
  void normalDelayChanged();
  void dontDelayChanged();

public:
  bool normalDelay;
  bool dontDelay;
};

class SAVEFILE_AUTOPORT WorldGeneral : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int lastBlackoutMap MEMBER lastBlackoutMap NOTIFY lastBlackoutMapChanged)
  Q_PROPERTY(int lastMap MEMBER lastMap NOTIFY lastMapChanged)
  Q_PROPERTY(Options* options MEMBER options NOTIFY optionsChanged)
  Q_PROPERTY(LetterDelay* letterDelay MEMBER letterDelay NOTIFY letterDelayChanged)

public:
  WorldGeneral(SaveFile* saveFile = nullptr);
  virtual ~WorldGeneral();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void lastBlackoutMapChanged();
  void lastMapChanged();
  void optionsChanged();
  void letterDelayChanged();

public slots:
  void reset();
  void randomize();

public:
  int lastBlackoutMap;
  int lastMap;
  Options* options;
  LetterDelay* letterDelay;
};

#endif // WORLDGENERAL_H
