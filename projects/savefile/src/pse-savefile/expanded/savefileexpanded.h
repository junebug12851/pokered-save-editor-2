/*
  * Copyright 2019 Twilight
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
#include "../savefile_autoport.h"

// Include the full type ONLY for the branches QML actually traverses
// (player / area / world / storage). dataExpanded.daycare / .hof / .rival are
// never read in QML, so they stay forward-declared + Q_DECLARE_OPAQUE_POINTER
// (in savefile_autoport.h) to keep this widely-included header lightweight.
// A forward-declared QObject pointer in a Q_PROPERTY reads as `undefined` in QML,
// which is fine for branches nothing traverses. See notes/reference/qt6-patterns.md.
#include "./player/player.h"
#include "./area/area.h"
#include "./world/world.h"
#include "./storage.h"

class SaveFile;
class Player;
class Area;
class World;
class Daycare;
class HallOfFame;
class Rival;
class Storage;

class SAVEFILE_AUTOPORT SaveFileExpanded : public QObject
{
  Q_OBJECT

  Q_PROPERTY(Player* player MEMBER player NOTIFY playerChanged)
  Q_PROPERTY(Area* area MEMBER area NOTIFY areaChanged)
  Q_PROPERTY(World* world MEMBER world NOTIFY worldChanged)
  Q_PROPERTY(Daycare* daycare MEMBER daycare NOTIFY daycareChanged)
  Q_PROPERTY(HallOfFame* hof MEMBER hof NOTIFY hofChanged)
  Q_PROPERTY(Rival* rival MEMBER rival NOTIFY rivalChanged)
  Q_PROPERTY(Storage* storage MEMBER storage NOTIFY storageChanged)

public:
  SaveFileExpanded(SaveFile* saveFile = nullptr);
  virtual ~SaveFileExpanded();

 