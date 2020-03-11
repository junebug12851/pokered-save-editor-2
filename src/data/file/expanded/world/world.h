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
#ifndef WORLD_H
#define WORLD_H

#include <QObject>

class SaveFile;

class WorldCompleted;
class WorldEvents;
class WorldGeneral;
class WorldHidden;
class WorldMissables;
class WorldOther;
class WorldScripts;
class WorldTowns;
class WorldTrades;
class WorldLocal;

class World : public QObject
{
  Q_OBJECT

  Q_PROPERTY(WorldCompleted* completed MEMBER completed NOTIFY completedChanged)
  Q_PROPERTY(WorldEvents* events MEMBER events NOTIFY eventsChanged)
  Q_PROPERTY(WorldGeneral* general MEMBER general NOTIFY generalChanged)
  Q_PROPERTY(WorldHidden* hidden MEMBER hidden NOTIFY hiddenChanged)
  Q_PROPERTY(WorldMissables* missables MEMBER missables NOTIFY missablesChanged)
  Q_PROPERTY(WorldOther* other MEMBER other NOTIFY otherChanged)
  Q_PROPERTY(WorldScripts* scripts MEMBER scripts NOTIFY scriptsChanged)
  Q_PROPERTY(WorldTowns* towns MEMBER towns NOTIFY townsChanged)
  Q_PROPERTY(WorldTrades* trades MEMBER trades NOTIFY tradesChanged)
  Q_PROPERTY(WorldLocal* local MEMBER local NOTIFY localChanged)

public:
  World(SaveFile* saveFile = nullptr);
  virtual ~World();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  void completedChanged();
  void eventsChanged();
  void generalChanged();
  void hiddenChanged();
  void missablesChanged();
  void otherChanged();
  void scriptsChanged();
  void townsChanged();
  void tradesChanged();
  void localChanged();

public slots:
  void reset();
  void randomize();

public:
  WorldCompleted* completed = nullptr;
  WorldEvents* events = nullptr;
  WorldGeneral* general = nullptr;
  WorldHidden* hidden = nullptr;
  WorldMissables* missables = nullptr;
  WorldOther* other = nullptr;
  WorldScripts* scripts = nullptr;
  WorldTowns* towns = nullptr;
  WorldTrades* trades = nullptr;
  WorldLocal* local = nullptr;
};

#endif // WORLD_H
