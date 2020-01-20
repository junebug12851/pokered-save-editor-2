/*
  * Copyright 2019 June Hanabi
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
#ifndef SAVEFILEEXPANDED_H
#define SAVEFILEEXPANDED_H

#include <QObject>

class SaveFile;
class Player;
class Area;
class World;
class Daycare;
class HallOfFame;
class Rival;
class Storage;

class SaveFileExpanded : public QObject
{
  Q_OBJECT

  Q_PROPERTY(Player* player_ MEMBER player NOTIFY playerChanged)
  Q_PROPERTY(Area* area_ MEMBER area NOTIFY areaChanged)
  Q_PROPERTY(World* world_ MEMBER world NOTIFY worldChanged)
  Q_PROPERTY(Daycare* daycare_ MEMBER daycare NOTIFY daycareChanged)
  Q_PROPERTY(HallOfFame* hof_ MEMBER hof NOTIFY hofChanged)
  Q_PROPERTY(Rival* rival_ MEMBER rival NOTIFY rivalChanged)
  Q_PROPERTY(Storage* storage_ MEMBER storage NOTIFY storageChanged)

public:
  SaveFileExpanded(SaveFile* saveFile = nullptr);
  virtual ~SaveFileExpanded();

signals:
  void playerChanged();
  void areaChanged();
  void worldChanged();
  void daycareChanged();
  void hofChanged();
  void rivalChanged();
  void storageChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

public:
  Player* player = nullptr;
  Area* area = nullptr;
  World* world = nullptr;
  Daycare* daycare = nullptr;
  HallOfFame* hof = nullptr;
  Rival* rival = nullptr;
  Storage* storage = nullptr;
};

#endif // SAVEFILEEXPANDED_H
