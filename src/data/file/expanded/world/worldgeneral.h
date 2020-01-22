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
#include "../../../../common/types.h"
class SaveFile;

class Options : public QObject {
  Q_OBJECT

  Q_PROPERTY(var8 textSlowness_ MEMBER textSlowness NOTIFY textSlownessChanged)
  Q_PROPERTY(bool battleStyleSet_ MEMBER battleStyleSet NOTIFY battleStyleSetChanged)
  Q_PROPERTY(bool battleAnimOff_ MEMBER battleAnimOff NOTIFY battleAnimOffChanged)

signals:
  void textSlownessChanged();
  void battleStyleSetChanged();
  void battleAnimOffChanged();

public:
  var8 textSlowness;
  bool battleStyleSet;
  bool battleAnimOff;
};

class LetterDelay : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool normalDelay_ MEMBER normalDelay NOTIFY normalDelayChanged)
  Q_PROPERTY(bool dontDelay_ MEMBER dontDelay NOTIFY dontDelayChanged)

signals:
  void normalDelayChanged();
  void dontDelayChanged();

public:
  bool normalDelay;
  bool dontDelay;
};

class WorldGeneral : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 lastBlackoutMap_ MEMBER lastBlackoutMap NOTIFY lastBlackoutMapChanged)
  Q_PROPERTY(var8 lastMap_ MEMBER lastMap NOTIFY lastMapChanged)
  Q_PROPERTY(Options* options_ MEMBER options NOTIFY optionsChanged)
  Q_PROPERTY(LetterDelay* letterDelay_ MEMBER letterDelay NOTIFY letterDelayChanged)

public:
  WorldGeneral(SaveFile* saveFile = nullptr);
  virtual ~WorldGeneral();

signals:
  void lastBlackoutMapChanged();
  void lastMapChanged();
  void optionsChanged();
  void letterDelayChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  var8 lastBlackoutMap;
  var8 lastMap;
  Options* options;
  LetterDelay* letterDelay;
};

#endif // WORLDGENERAL_H
