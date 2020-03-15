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
  Q_PROPERTY(MapDBEntry* pickRandom READ MapDBEntry*)
  Q_PROPERTY(MapSearch* startOver READ MapSearch*)
  Q_PROPERTY(MapSearch* hasConnections READ MapSearch*)
  Q_PROPERTY(MapSearch* noConnections READ MapSearch*)
  Q_PROPERTY(MapSearch* hasWarpsOut READ MapSearch*)
  Q_PROPERTY(MapSearch* noWarpsOut READ MapSearch*)
  Q_PROPERTY(MapSearch* hasWarpsIn READ MapSearch*)
  Q_PROPERTY(MapSearch* noWarpsIn READ MapSearch*)
  Q_PROPERTY(MapSearch* hasSigns READ MapSearch*)
  Q_PROPERTY(MapSearch* noSigns READ MapSearch*)
  Q_PROPERTY(MapSearch* hasSprites READ MapSearch*)
  Q_PROPERTY(MapSearch* noSprites READ MapSearch*)
  Q_PROPERTY(MapSearch* hasSpriteSet READ MapSearch*)
  Q_PROPERTY(MapSearch* noSpriteSet READ MapSearch*)
  Q_PROPERTY(MapSearch* hasDynamicSpriteSet READ MapSearch*)
  Q_PROPERTY(MapSearch* noDynamicSpriteSet READ MapSearch*)
  Q_PROPERTY(MapSearch* hasMons READ MapSearch*)
  Q_PROPERTY(MapSearch* noMons READ MapSearch*)
  Q_PROPERTY(MapSearch* isIncomplete READ MapSearch*)
  Q_PROPERTY(MapSearch* notIncomplete READ MapSearch*)
  Q_PROPERTY(MapSearch* isGlitch READ MapSearch*)
  Q_PROPERTY(MapSearch* notGlitch READ MapSearch*)
  Q_PROPERTY(MapSearch* isSpsecial READ MapSearch*)
  Q_PROPERTY(MapSearch* notSpecial READ MapSearch*)
  Q_PROPERTY(MapSearch* isGood READ MapSearch*)
  Q_PROPERTY(MapSearch* isCity READ MapSearch*)
  Q_PROPERTY(MapSearch* notCity READ MapSearch*)

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
  MapSearch* isCity();
  MapSearch* notCity();

  // QML Interface
  const QVector<MapDBEntry*> getMaps() const;
  int getMapCount() const;
  Q_INVOKABLE const MapDBEntry* mapAt(const int ind) const;

public slots:
  void engineProtect(const QQmlEngine* const engine) const;

private slots:
  void engineRegister() const;

private:
  QVector<MapDBEntry*> results;
};

#endif // MAPSEARCH_H
