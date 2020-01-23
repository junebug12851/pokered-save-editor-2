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
#ifndef MAPSEARCH_H
#define MAPSEARCH_H

#include <QMetaType>
#include <QString>
#include <QVector>
#include "../../common/types.h"

class MapDBEntry;

class MapSearch
{
public:
  MapSearch();

  MapDBEntry* pickRandom();
  MapSearch* startOver();

  MapSearch* notNamed(QString val);
  MapSearch* indexLt(var8 val);
  MapSearch* indexGt(var8 val);
  MapSearch* widthGt(var8 val);
  MapSearch* widthLt(var8 val);
  MapSearch* heightGt(var8 val);
  MapSearch* heightLt(var8 val);
  MapSearch* areaGt(var8 val);
  MapSearch* areaLt(var8 val);
  MapSearch* hasTileset(QString val);
  MapSearch* notTileset(QString val);
  MapSearch* isType(QString val);
  MapSearch* notType(QString val);
  MapSearch* hasConnections();
  MapSearch* noConnections();
  MapSearch* hasWarpsOut();
  MapSearch* noWarpsOut();
  MapSearch* hasWarpsIn();
  MapSearch* noWarpsIn();
  MapSearch* hasSigns();
  MapSearch* noSigns();
  MapSearch* hasSprites();
  MapSearch* noSprites();
  MapSearch* hasSpriteSet();
  MapSearch* noSpriteSet();
  MapSearch* hasDynamicSpriteSet();
  MapSearch* noDynamicSpriteSet();
  MapSearch* hasMons();
  MapSearch* noMons();
  MapSearch* isIncomplete();
  MapSearch* notIncomplete();
  MapSearch* isGlitch();
  MapSearch* notGlitch();
  MapSearch* isSpsecial();
  MapSearch* notSpecial();

  // * A normal non-special or glitch map
  // * A map that's complete (Not an incomplete map)
  // * Has at least one warp in and out (You have to be able to enter and leave)
  // * Is not the strange elevator that has an invalid warp
  MapSearch* isGood();
  MapSearch* isCity();
  MapSearch* notCity();

  QVector<MapDBEntry*> results;
};

Q_DECLARE_METATYPE(MapSearch)

#endif // MAPSEARCH_H
