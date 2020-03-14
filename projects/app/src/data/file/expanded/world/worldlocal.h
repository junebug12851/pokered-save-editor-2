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
#ifndef WORLDPUZZLE_H
#define WORLDPUZZLE_H

#include <QObject>
#include <pse-common/types.h>

class SaveFile;

class WorldLocal : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int lock1 MEMBER lock1 NOTIFY lock1Changed)
  Q_PROPERTY(int lock2 MEMBER lock2 NOTIFY lock2Changed)
  Q_PROPERTY(int quizOpp MEMBER quizOpp NOTIFY quizOppChanged)
  Q_PROPERTY(bool safariGameOver MEMBER safariGameOver NOTIFY safariGameOverChanged)
  Q_PROPERTY(int safariBallCount MEMBER safariBallCount NOTIFY safariBallCountChanged)
  Q_PROPERTY(int safariSteps MEMBER safariSteps NOTIFY safariStepsChanged)

public:
  WorldLocal(SaveFile* saveFile = nullptr);
  virtual ~WorldLocal();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void lock1Changed();
  void lock2Changed();
  void quizOppChanged();
  void safariGameOverChanged();
  void safariBallCountChanged();
  void safariStepsChanged();

public slots:
  void reset();
  void randomize();

public:
  // Lt. Surge Trash Can Locks
  int lock1;
  int lock2;

  // Cinnabar Gym Next Opp
  int quizOpp;

  // Safari
  bool safariGameOver;
  int safariBallCount;
  int safariSteps;
};

#endif // WORLDPUZZLE_H
