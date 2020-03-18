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
#ifndef MAPDBENTRY_H
#define MAPDBENTRY_H

#include <QObject>
#include <QHash>
#include <QVector>

#include "../db_autoport.h"

class QQmlEngine;
class MapsDB;
class MapDBEntryWarpOut;
class MapDBEntryWarpIn;
class MapDBEntrySign;
class MapDBEntrySprite;
class MapDBEntryConnect;
class MapDBEntryWildMon;
class SpriteSetDBEntry;
class MusicDBEntry;
class TilesetDBEntry;
class EventDBEntry;
class FlyDBEntry;
class HiddenItemDBEntry;
class ScriptDBEntry;

struct DB_AUTOPORT MapDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString bestName READ bestName CONSTANT)
  Q_PROPERTY(int height2X2 READ height2X2 CONSTANT)
  Q_PROPERTY(int width2X2 READ width2X2 CONSTANT)
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(int getInd READ getInd CONSTANT)
  Q_PROPERTY(bool getGlitch READ getGlitch CONSTANT)
  Q_PROPERTY(bool getSpecial READ getSpecial CONSTANT)
  Q_PROPERTY(int getWarpOutSize READ getWarpOutSize CONSTANT)
  Q_PROPERTY(int getWarpInSize READ getWarpInSize CONSTANT)
  Q_PROPERTY(int getSignsSize READ getSignsSize CONSTANT)
  Q_PROPERTY(int getSpritesSize READ getSpritesSize CONSTANT)
  Q_PROPERTY(int getMonRate READ getMonRate CONSTANT)
  Q_PROPERTY(int getMonRateWater READ getMonRateWater CONSTANT)
  Q_PROPERTY(int getMonsRedSize READ getMonsRedSize CONSTANT)
  Q_PROPERTY(int getMonsBlueSize READ getMonsBlueSize CONSTANT)
  Q_PROPERTY(int getMonsWaterSize READ getMonsWaterSize CONSTANT)
  Q_PROPERTY(int getSpriteSet READ getSpriteSet CONSTANT)
  Q_PROPERTY(SpriteSetDBEntry* getToSpriteSet READ getToSpriteSet CONSTANT)
  Q_PROPERTY(int getBorder READ getBorder CONSTANT)
  Q_PROPERTY(int getBank READ getBank CONSTANT)
  Q_PROPERTY(int getDataPtr READ getDataPtr CONSTANT)
  Q_PROPERTY(int getScriptPtr READ getScriptPtr CONSTANT)
  Q_PROPERTY(int getTextPtr READ getTextPtr CONSTANT)
  Q_PROPERTY(int getWidth READ getWidth CONSTANT)
  Q_PROPERTY(int getHeight READ getHeight CONSTANT)
  Q_PROPERTY(QString getMusic READ getMusic CONSTANT)
  Q_PROPERTY(QString getTileset READ getTileset CONSTANT)
  Q_PROPERTY(QString getModernName READ getModernName CONSTANT)
  Q_PROPERTY(QString getIncomplete READ getIncomplete CONSTANT)
  Q_PROPERTY(MusicDBEntry* getToMusic READ getToMusic CONSTANT)
  Q_PROPERTY(TilesetDBEntry* getToTileset READ getToTileset CONSTANT)
  Q_PROPERTY(MapDBEntry* getToComplete READ getToComplete CONSTANT)
  Q_PROPERTY(int getToEventsSize READ getToEventsSize CONSTANT)
  Q_PROPERTY(FlyDBEntry* getToFlyDestination READ getToFlyDestination CONSTANT)
  Q_PROPERTY(int getToHiddenItemsSize READ getToHiddenItemsSize CONSTANT)
  Q_PROPERTY(ScriptDBEntry* getToScript READ getToScript CONSTANT)

public:
  // Provides the best display name
  const QString bestName() const;

  // These have been removed from the JSON data because they are simply
  // dimensions times 2 and thus redundant and repetitive to inlclude in JSON
  int height2X2() const;
  int width2X2() const;

  const QString getName() const;
  int getInd() const;
  bool getGlitch() const;
  bool getSpecial() const;

  const QVector<MapDBEntryWarpOut*> getWarpOut() const;
  int getWarpOutSize() const;
  Q_INVOKABLE const MapDBEntryWarpOut* getWarpOutAt(const int ind) const;

  const QVector<MapDBEntryWarpIn*> getWarpIn() const;
  int getWarpInSize() const;
  Q_INVOKABLE const MapDBEntryWarpIn* getWarpInAt(const int ind) const;

  const QVector<MapDBEntrySign*> getSigns() const;
  int getSignsSize() const;
  Q_INVOKABLE const MapDBEntrySign* getSignsAt(const int ind) const;

  const QVector<MapDBEntrySprite*> getSprites() const;
  int getSpritesSize() const;
  Q_INVOKABLE const MapDBEntrySprite* getSpritesAt(const int ind) const;

  const QHash<int, MapDBEntryConnect*> getConnect() const;
  Q_INVOKABLE const MapDBEntryConnect* getConnectAt(const int val) const;

  int getMonRate() const;
  int getMonRateWater() const;

  const QVector<MapDBEntryWildMon*> getMonsRed() const;
  int getMonsRedSize() const;
  Q_INVOKABLE const MapDBEntryWildMon* getMonsRedAt(const int ind) const;

  const QVector<MapDBEntryWildMon*> getMonsBlue() const;
  int getMonsBlueSize() const;
  Q_INVOKABLE const MapDBEntryWildMon* getMonsBlueAt(const int ind) const;

  const QVector<MapDBEntryWildMon*> getMonsWater() const;
  int getMonsWaterSize() const;
  Q_INVOKABLE const MapDBEntryWildMon* getMonsWaterAt(const int ind) const;

  int getSpriteSet() const;
  const SpriteSetDBEntry* getToSpriteSet() const;
  int getBorder() const;
  int getBank() const;
  int getDataPtr() const;
  int getScriptPtr() const;
  int getTextPtr() const;
  int getWidth() const;
  int getHeight() const;
  const QString getMusic() const;
  const QString getTileset() const;
  const QString getModernName() const;
  const QString getIncomplete() const;
  const MusicDBEntry* getToMusic() const;
  const TilesetDBEntry* getToTileset() const;
  const MapDBEntry* getToComplete() const;

  const QVector<EventDBEntry*> getToEvents() const;
  int getToEventsSize() const;
  Q_INVOKABLE const EventDBEntry* getToEventsAt(const int ind) const;

  const FlyDBEntry* getToFlyDestination() const;

  const QVector<HiddenItemDBEntry*> getToHiddenItems() const;
  int getToHiddenItemsSize() const;
  Q_INVOKABLE const HiddenItemDBEntry* getToHiddenItemsAt(const int ind) const;

  const ScriptDBEntry* getToScript() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  MapDBEntry();
  MapDBEntry(const QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  // Optional bool values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  QString name = "";
  int ind = -1;

  bool glitch = false;
  bool special = false;

  // Warps to other maps
  QVector<MapDBEntryWarpOut*> warpOut;

  // Warps In from other maps
  QVector<MapDBEntryWarpIn*> warpIn;

  // Signs on map
  QVector<MapDBEntrySign*> signs;

  // Sprites on map
  QVector<MapDBEntrySprite*> sprites;

  // Connecting Maps
  QHash<int, MapDBEntryConnect*> connect;

  // Wild Pokemon Encounter Rate
  // Along with mons for Red & Blue & Water Mons
  // Although there is strangely only 1 map in the game that carries both
  // water and land Pokemon. More strangely there's only 1 set of water Pokemon
  // that all maps share that have water
  int monRate = -1;
  int monRateWater = -1;
  QVector<MapDBEntryWildMon*> monsRed;
  QVector<MapDBEntryWildMon*> monsBlue;
  QVector<MapDBEntryWildMon*> monsWater;

  // Sprite Set
  int spriteSet = -1;
  SpriteSetDBEntry* toSpriteSet = nullptr;

  // Border Block #
  int border = -1;

  int bank = -1;
  int dataPtr = -1;
  int scriptPtr = -1;
  int textPtr = -1;
  int width = -1;
  int height = -1;

  QString music = "";
  QString tileset = "";
  QString modernName = "";
  QString incomplete = "";

  // Deep Linking
  MusicDBEntry* toMusic = nullptr; // To Map Music
  TilesetDBEntry* toTileset = nullptr; // To Map Tileset
  MapDBEntry* toComplete = nullptr; // To Complete Version of Map
  QVector<EventDBEntry*> toEvents; // To Associated Events
  FlyDBEntry* toFlyDestination = nullptr; // To Associated Fly Destination
  QVector<HiddenItemDBEntry*> toHiddenItems; // To Associated Hidden Items
  ScriptDBEntry* toScript = nullptr;

  friend class MapsDB;
};

#endif // MAPDBENTRY_H
