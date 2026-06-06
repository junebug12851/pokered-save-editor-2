/*
  * Copyright 2020 Twilight
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
#include <QString>
#include <QVector>

#include "../db_autoport.h"

class MapDBEntry;
class QQmlEngine;

class DB_AUTOPORT MapSearch : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int mapCount READ getMapCount NOTIFY mapCountChanged STORED false)
  Q_PROPERTY(MapDBEntry* pickRandom READ pickRandom STORED false)
  Q_PROPERTY(MapSearch* startOver READ startOver STORED false)
  Q_PROPERTY(MapSearch* hasConnections READ hasConnections STORED false)
  Q_PROPERTY(MapSearch* noConnections READ noConnections STORED false)
  Q_PROPERTY(MapSearch* hasWarpsOut READ hasWarpsOut STORED false)
  Q_PROPERTY(MapSearch* noWarpsOut READ noWarpsOut STORED false)
  Q_PROPERTY(MapSearch* hasWarpsIn READ hasWarpsIn STORED false)
  Q_PROPERTY(MapSearch* noWarpsIn READ noWarpsIn STORED false)
  Q_PROPERTY(MapSearch* hasSigns READ hasSigns STORED false)
  Q_PROPERTY(MapSearch* noSigns READ noSigns STORED false)
  Q_PROPERTY(MapSearch* hasSprites READ hasSprites STORED false)
  Q_PROPERTY(MapSearch* noSprites READ noSprites STORED false)
  Q_PROPERTY(MapSearch* hasSpriteSet READ hasSpriteSet STORED false)
  Q_PROPERTY(MapSearch* noSpriteSet READ noSpriteSet STORED false)
  Q_PROPERTY(MapSearch* hasDynamicSpriteSet READ hasDynamicSpriteSet STORED false)
  Q_PROPERTY(MapSearch* noDynamicSpriteSet READ noDynamicSpriteSet STORED false)
  Q_PROPERTY(MapSearch* hasMons READ hasMons STORED false)
  Q_PROPERTY(MapSearch* noMons READ noMons STORED false)
  Q_PROPERTY(MapSearch* isIncomplete READ isIncomplete STORED false)
  Q_PROPERTY(MapSearch* notIncomplete READ notIncomplete STORED false)
  Q_PROPERTY(MapSearch* isGlitch READ isGlitch STORED false)
  Q_PROPERTY(MapSearch* notGlitch READ notGlitch STORED false)
  Q_PROPERTY(MapSearch* isSpsecial READ isSpsecial STORED false)
  Q_PROPERTY(MapSearch* notSpecial READ notSpecial STORED false)
  Q_PROPERTY(MapSearch* isGood READ isGood STORED false)
  Q_PROPERTY(MapSearch* isCity READ isCity STORED false)
  Q_PROPERTY(MapSearch* notCity READ notCity STORED false)

signals:
  void mapCountChanged();

public:
  MapSearch();

  MapDBEntry* pickRandom();
  MapSearch* startOver();

  Q_INVOKABLE MapSearch* notNamed(QString val);
  Q_INVOKABLE MapSearch* indexLt(int val);
  Q_INVOKABLE MapSearch* indexGt(int val);
  Q_INVOKABLE MapSearch* widthGt(int val);
  Q_INVOKABLE MapSearch* widthLt(int val);
  Q_INVOKABLE MapSearch* heightGt(int val);
  Q_INVOKABLE MapSearch* heightLt(int val);
  Q_INVOKABLE MapSearch* areaGt(int val);
  Q_INVOKABLE MapSearch* areaLt(int val);
  Q_INVOKABLE MapSearch* hasTileset(QString val);
  Q_INVOKABLE MapSearch* notTileset(QString val);
  Q_INVOKABLE MapSearch* isType(QString val);
  Q_INVOKABLE MapSearch* notType(QString val);
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
  MapSearch* isNotBad();
  MapSearch* isCity();
  MapSearch* notCity();

  // QML Interface
  const QVector<MapDBEntry*> getMaps() const;
  int getMapCount() const;
  Q_INVOKABLE const MapDBEntry* mapAt(const int ind) const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  QVector<MapDBEntry*> results;
};

