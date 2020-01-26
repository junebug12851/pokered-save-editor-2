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
#ifndef AREAGENERAL_H
#define AREAGENERAL_H

#include <QObject>
#include "../../../../common/types.h"
class SaveFile;

struct ContrastIds : public QObject
{
  Q_OBJECT
  Q_ENUMS(ContrastIds_)

public:
  enum ContrastIds_ : int
  {
    Normal = 0,
    Darken1 = 3,
    Darken2_NeedsFlash = 6,
    Darken3_SolidBlack = 9,

    Glitch_1A = 1,
    Glitch_1B = 2,
    Glitch_2A = 4,
    Glitch_2B = 5,
    Glitch_3A = 7,
    Glitch_3B = 8
  };
};

class AreaGeneral : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int contrast MEMBER contrast NOTIFY contrastChanged)
  Q_PROPERTY(bool noLetterDelay MEMBER noLetterDelay NOTIFY noLetterDelayChanged)
  Q_PROPERTY(bool countPlaytime MEMBER countPlaytime NOTIFY countPlaytimeChanged)

public:
  AreaGeneral(SaveFile* saveFile = nullptr);
  virtual ~AreaGeneral();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void contrastChanged();
  void noLetterDelayChanged();
  void countPlaytimeChanged();

public slots:
  void reset();
  void randomize();

public:
  int contrast;
  bool noLetterDelay;
  bool countPlaytime;
};

#endif // AREAGENERAL_H
