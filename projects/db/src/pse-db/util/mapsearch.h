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

/**
 * @brief A chainable filter ("finder") over the maps -- the heart of map randomization.
 *
 * Obtained from MapsDB (search()/searchRaw()). Starts with every map in
 * @ref results and narrows them with chainable predicates; each returns @c this
 * for fluent chaining. Two flavours of predicate:
 * - the no-argument filters are exposed as @b Q_PROPERTY (so QML can chain them as
 *   property reads, e.g. `search.hasWarpsOut.hasWarpsIn`);
 * - the parameterised ones (`indexGt`, `hasTileset`, ...) are @b Q_INVOKABLE.
 *
 * pickRandom() returns a random map from the current results -- the primary tool
 * the "place player on any valid map" feature uses (typically after `isGood()`).
 *
 * @see MapsDB, MapDBEntry, FontSearch (the analogous font finder).
 */
class DB_AUTOPORT MapSearch : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int mapCount READ getMapCount NOTIFY mapCountChanged STORED false) ///< Live count of current results.
  Q_PROPERTY(MapDBEntry* pickRandom READ pickRandom STORED false)              ///< A random map from the results.
  Q_PROPERTY(MapSearch* startOver READ startOver STORED false)                 ///< Reset to all maps (chainable).
  Q_PROPERTY(MapSearch* hasConnections READ hasConnections STORED false)       ///< Keep maps with edge connections.
  Q_PROPERTY(MapSearch* noConnections READ noConnections STORED false)         ///< Keep maps without connections.
  Q_PROPERTY(MapSearch* hasWarpsOut READ hasWarpsOut STORED false)             ///< Keep maps with outgoing warps.
  Q_PROPERTY(MapSearch* noWarpsOut READ noWarpsOut STORED false)               ///< Keep maps without outgoing warps.
  Q_PROPERTY(MapSearch* hasWarpsIn READ hasWarpsIn STORED false)               ///< Keep maps with incoming warps.
  Q_PROPERTY(MapSearch* noWarpsIn READ noWarpsIn STORED false)                 ///< Keep maps without incoming warps.
  Q_PROPERTY(MapSearch* hasSigns READ hasSigns STORED false)                   ///< Keep maps with signs.
  Q_PROPERTY(MapSearch* noSigns READ noSigns STORED false)                     ///< Keep maps without signs.
  Q_PROPERTY(MapSearch* hasSprites READ hasSprites STORED false)               ///< Keep maps with sprites.
  Q_PROPERTY(MapSearch* noSprites READ noSprites STORED false)                 ///< Keep maps without sprites.
  Q_PROPERTY(MapSearch* hasSpriteSet READ hasSpriteSet STORED false)           ///< Keep maps with a sprite set.
  Q_PROPERTY(MapSearch* noSpriteSet READ noSpriteSet STORED false)             ///< Keep maps without a sprite set.
  Q_PROPERTY(MapSearch* hasDynamicSpriteSet READ hasDynamicSpriteSet STORED false) ///< Keep maps with a split sprite set.
  Q_PROPERTY(MapSearch* noDynamicSpriteSet READ noDynamicSpriteSet STORED false)   ///< Keep maps without a split sprite set.
  Q_PROPERTY(MapSearch* hasMons READ hasMons STORED false)                     ///< Keep maps with wild encounters.
  Q_PROPERTY(MapSearch* noMons READ noMons STORED false)                       ///< Keep maps without wild encounters.
  Q_PROPERTY(MapSearch* isIncomplete READ isIncomplete STORED false)           ///< Keep incomplete maps.
  Q_PROPERTY(MapSearch* notIncomplete READ notIncomplete STORED false)         ///< Drop incomplete maps.
  Q_PROPERTY(MapSearch* isGlitch READ isGlitch STORED false)                   ///< Keep glitch maps.
  Q_PROPERTY(MapSearch* notGlitch READ notGlitch STORED false)                 ///< Drop glitch maps.
  Q_PROPERTY(MapSearch* isSpsecial READ isSpsecial STORED false)               ///< Keep special maps (sic).
  Q_PROPERTY(MapSearch* notSpecial READ notSpecial STORED false)               ///< Drop special maps.
  Q_PROPERTY(MapSearch* isGood READ isGood STORED false)                       ///< Keep only "good" playable maps (see method note).
  Q_PROPERTY(MapSearch* isCity READ isCity STORED false)                       ///< Keep cities/towns.
  Q_PROPERTY(MapSearch* notCity READ notCity STORED false)                     ///< Drop cities/towns.

signals:
  void mapCountChanged(); ///< Result count changed (after a filter).

public:
  MapSearch(); ///< Start with all maps in the result set.

  MapDBEntry* pickRandom();  ///< A random map from the current results (backs @c pickRandom).
  MapSearch* startOver();    ///< Reset to all maps. Returns @c this.

  Q_INVOKABLE MapSearch* notNamed(QString val);    ///< Drop the map named @p val.
  Q_INVOKABLE MapSearch* indexLt(int val);         ///< Keep maps with index < @p val.
  Q_INVOKABLE MapSearch* indexGt(int val);         ///< Keep maps with index > @p val.
  Q_INVOKABLE MapSearch* widthGt(int val);         ///< Keep maps wider than @p val.
  Q_INVOKABLE MapSearch* widthLt(int val);         ///< Keep maps narrower than @p val.
  Q_INVOKABLE MapSearch* heightGt(int val);        ///< Keep maps taller than @p val.
  Q_INVOKABLE MapSearch* heightLt(int val);        ///< Keep maps shorter than @p val.
  Q_INVOKABLE MapSearch* areaGt(int val);          ///< Keep maps with area > @p val.
  Q_INVOKABLE MapSearch* areaLt(int val);          ///< Keep maps with area < @p val.
  Q_INVOKABLE MapSearch* hasTileset(QString val);  ///< Keep maps using tileset @p val.
  Q_INVOKABLE MapSearch* notTileset(QString val);  ///< Drop maps using tileset @p val.
  Q_INVOKABLE MapSearch* isType(QString val);      ///< Keep maps of tileset-type @p val.
  Q_INVOKABLE MapSearch* notType(QString val);     ///< Drop maps of tileset-type @p val.
  MapSearch* hasConnections();      ///< @see hasConnections property.
  MapSearch* noConnections();       ///< @see noConnections property.
  MapSearch* hasWarpsOut();         ///< @see hasWarpsOut property.
  MapSearch* noWarpsOut();          ///< @see noWarpsOut property.
  MapSearch* hasWarpsIn();          ///< @see hasWarpsIn property.
  MapSearch* noWarpsIn();           ///< @see noWarpsIn property.
  MapSearch* hasSigns();            ///< @see hasSigns property.
  MapSearch* noSigns();             ///< @see noSigns property.
  MapSearch* hasSprites();          ///< @see hasSprites property.
  MapSearch* noSprites();           ///< @see noSprites property.
  MapSearch* hasSpriteSet();        ///< @see hasSpriteSet property.
  MapSearch* noSpriteSet();         ///< @see noSpriteSet property.
  MapSearch* hasDynamicSpriteSet(); ///< @see hasDynamicSpriteSet property.
  MapSearch* noDynamicSpriteSet();  ///< @see noDynamicSpriteSet property.
  MapSearch* hasMons();             ///< @see hasMons property.
  MapSearch* noMons();              ///< @see noMons property.
  MapSearch* isIncomplete();        ///< @see isIncomplete property.
  MapSearch* notIncomplete();       ///< @see notIncomplete property.
  MapSearch* isGlitch();            ///< @see isGlitch property.
  MapSearch* notGlitch();           ///< @see notGlitch property.
  MapSearch* isSpsecial();          ///< @see isSpsecial property (sic).
  MapSearch* notSpecial();          ///< @see notSpecial property.

  // * A normal non-special or glitch map
  // * A map that's complete (Not an incomplete map)
  // * Has at least one warp in and out (You have to be able to enter and leave)
  // * Is not the strange elevator that has an invalid warp
  MapSearch* isGood();   ///< Keep only "good" maps (criteria listed above).
  MapSearch* isNotBad(); ///< The complement helper used by isGood().
  MapSearch* isCity();   ///< @see isCity property.
  MapSearch* notCity();  ///< @see notCity property.

  // QML Interface
  const QVector<MapDBEntry*> getMaps() const; ///< The current result set.
  int getMapCount() const;                    ///< Result count (backs @c mapCount).
  Q_INVOKABLE const MapDBEntry* mapAt(const int ind) const; ///< Result @p ind (for QML).

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  QVector<MapDBEntry*> results; ///< The current (narrowed) map set.
};
