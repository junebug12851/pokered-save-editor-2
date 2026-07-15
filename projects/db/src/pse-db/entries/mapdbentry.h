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
#include <QHash>
#include <QVector>

#include "../db_autoport.h"

class QQmlEngine;
class MapsDB;
class MapDBEntryWarpOut;
class MapDBEntryWarpIn;
class MapDBEntrySign;
class MapDBEntryText;
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

/**
 * @brief One named step of a map's scripted-event sequence -- a row of its `_ScriptPointers`
 *        table, imported from pret/pokered by scripts/import_map_scripts.py into maps.json.
 *
 * `id` is the 0-based index the save's `wCurMapScript` (0x2CE5) holds; `label` is the friendly
 * name for the "Current script" ComboBox. A plain struct (no QObject) -- MapModel turns the list
 * into a QVariantList for the picker, so QML never touches it directly.
 * See notes/reference/area-map-state.md.
 */
struct MapScriptStep {
  int id = 0;    ///< 0-based step index == the wCurMapScript value.
  QString name;  ///< The SCRIPT_<MAP>_<NAME> constant tail.
  QString label; ///< Friendly label for the picker.
};

/**
 * @brief One map's complete static definition -- the root of the MapDBEntry family.
 *
 * The DB counterpart to the save's Area: everything canonical about a map. It owns
 * the map's warps (in/out), signs, sprites, edge connections, and wild-encounter
 * tables (red/blue/water), plus its pointers, sprite-set, music, and tileset.
 * deepLink() then resolves a wide web of cross-references (the `to*` members):
 * music, tileset, the "complete" version of an incomplete map, associated events,
 * fly destination, hidden items, and script.
 *
 * Sub-lists are surfaced to QML via the `...Size` + invokable `...At()` accessor
 * workaround. This is the largest entry family in the project; the sub-types
 * (warps, sign, sprite + its 4 subclasses, connect, wildmon) each have their own
 * header. See db.md.
 *
 * @see MapsDB, Area (the save-side map), and the MapDBEntry* family.
 */
struct DB_AUTOPORT MapDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString bestName READ bestName CONSTANT)         ///< Best display name (modern/internal).
  Q_PROPERTY(int height2X2 READ height2X2 CONSTANT)           ///< Height in 2x2 units (derived).
  Q_PROPERTY(int width2X2 READ width2X2 CONSTANT)             ///< Width in 2x2 units (derived).
  Q_PROPERTY(QString getName READ getName CONSTANT)           ///< Internal name.
  Q_PROPERTY(int getInd READ getInd CONSTANT)                 ///< Map index.
  Q_PROPERTY(bool getGlitch READ getGlitch CONSTANT)          ///< Glitch map.
  Q_PROPERTY(bool getSpecial READ getSpecial CONSTANT)        ///< Special map.
  Q_PROPERTY(int getWarpOutSize READ getWarpOutSize CONSTANT)     ///< Number of outgoing warps.
  Q_PROPERTY(int getWarpInSize READ getWarpInSize CONSTANT)       ///< Number of incoming warps.
  Q_PROPERTY(int getSignsSize READ getSignsSize CONSTANT)         ///< Number of signs.
  Q_PROPERTY(int getTextEntriesSize READ getTextEntriesSize CONSTANT) ///< Number of text-table entries.
  Q_PROPERTY(int getSpritesSize READ getSpritesSize CONSTANT)     ///< Number of sprites.
  Q_PROPERTY(int getMonRate READ getMonRate CONSTANT)             ///< Land encounter rate.
  Q_PROPERTY(int getMonRateWater READ getMonRateWater CONSTANT)   ///< Water encounter rate.
  Q_PROPERTY(int getMonsRedSize READ getMonsRedSize CONSTANT)     ///< Red-version wild count.
  Q_PROPERTY(int getMonsBlueSize READ getMonsBlueSize CONSTANT)   ///< Blue-version wild count.
  Q_PROPERTY(int getMonsWaterSize READ getMonsWaterSize CONSTANT) ///< Water wild count.
  Q_PROPERTY(int getSpriteSet READ getSpriteSet CONSTANT)         ///< Sprite-set index.
  Q_PROPERTY(SpriteSetDBEntry* getToSpriteSet READ getToSpriteSet CONSTANT) ///< Resolved sprite set.
  Q_PROPERTY(int getBorder READ getBorder CONSTANT)               ///< Border block number.
  Q_PROPERTY(int getBank READ getBank CONSTANT)                   ///< Map data bank.
  Q_PROPERTY(int getDataPtr READ getDataPtr CONSTANT)             ///< Map data pointer.
  Q_PROPERTY(int getScriptPtr READ getScriptPtr CONSTANT)         ///< Map script pointer.
  Q_PROPERTY(int getTextPtr READ getTextPtr CONSTANT)             ///< Map text pointer.
  Q_PROPERTY(int getWidth READ getWidth CONSTANT)                 ///< Width (blocks).
  Q_PROPERTY(int getHeight READ getHeight CONSTANT)               ///< Height (blocks).
  Q_PROPERTY(QString getMusic READ getMusic CONSTANT)             ///< Music track name.
  Q_PROPERTY(QString getTileset READ getTileset CONSTANT)         ///< Tileset name.
  Q_PROPERTY(QString getModernName READ getModernName CONSTANT)   ///< Modern display name.
  Q_PROPERTY(QString getIncomplete READ getIncomplete CONSTANT)   ///< Incomplete-map marker.
  Q_PROPERTY(MusicDBEntry* getToMusic READ getToMusic CONSTANT)         ///< Resolved music.
  Q_PROPERTY(TilesetDBEntry* getToTileset READ getToTileset CONSTANT)   ///< Resolved tileset.
  Q_PROPERTY(MapDBEntry* getToComplete READ getToComplete CONSTANT)     ///< Resolved complete-map version.
  Q_PROPERTY(int getToEventsSize READ getToEventsSize CONSTANT)         ///< Number of associated events.
  Q_PROPERTY(FlyDBEntry* getToFlyDestination READ getToFlyDestination CONSTANT) ///< Resolved fly destination.
  Q_PROPERTY(int getToHiddenItemsSize READ getToHiddenItemsSize CONSTANT) ///< Number of hidden items here.
  Q_PROPERTY(ScriptDBEntry* getToScript READ getToScript CONSTANT)     ///< Resolved script.

public:
  // Provides the best display name
  const QString bestName() const; ///< @see bestName property.

  // These have been removed from the JSON data because they are simply
  // dimensions times 2 and thus redundant and repetitive to inlclude in JSON
  int height2X2() const; ///< Height x2 (derived; see note).
  int width2X2() const;  ///< Width x2 (derived; see note).

  const QString getName() const; ///< @see getName property.
  int getInd() const;            ///< @see getInd property.
  bool getGlitch() const;        ///< @see getGlitch property.
  bool getSpecial() const;       ///< @see getSpecial property.

  const QVector<MapDBEntryWarpOut*> getWarpOut() const; ///< Outgoing warps.
  int getWarpOutSize() const;                           ///< @see getWarpOutSize property.
  Q_INVOKABLE const MapDBEntryWarpOut* getWarpOutAt(const int ind) const; ///< Outgoing warp @p ind (for QML).

  const QVector<MapDBEntryWarpIn*> getWarpIn() const;   ///< Incoming warps.
  int getWarpInSize() const;                            ///< @see getWarpInSize property.
  Q_INVOKABLE const MapDBEntryWarpIn* getWarpInAt(const int ind) const; ///< Incoming warp @p ind (for QML).

  const QVector<MapDBEntrySign*> getSigns() const;      ///< Signs on the map.
  int getSignsSize() const;                             ///< @see getSignsSize property.
  Q_INVOKABLE const MapDBEntrySign* getSignsAt(const int ind) const; ///< Sign @p ind (for QML).

  const QVector<MapDBEntryText*> getTextEntries() const; ///< The map's text-pointer table (id -> words + category).
  int getTextEntriesSize() const;                        ///< @see getTextEntriesSize property.
  Q_INVOKABLE const MapDBEntryText* getTextEntriesAt(const int ind) const; ///< Text entry @p ind, 0-based (for QML).

  /// The map's named script steps (the "Current script" picker source). Empty for unscripted maps.
  const QVector<MapScriptStep>& getScriptSteps() const;

  const QVector<MapDBEntrySprite*> getSprites() const;  ///< Sprites on the map.
  int getSpritesSize() const;                           ///< @see getSpritesSize property.
  Q_INVOKABLE const MapDBEntrySprite* getSpritesAt(const int ind) const; ///< Sprite @p ind (for QML).

  const QHash<int, MapDBEntryConnect*> getConnect() const; ///< Edge connections by direction.
  Q_INVOKABLE const MapDBEntryConnect* getConnectAt(const int val) const; ///< Connection in direction @p val (for QML).

  int getMonRate() const;      ///< @see getMonRate property.
  int getMonRateWater() const; ///< @see getMonRateWater property.

  const QVector<MapDBEntryWildMon*> getMonsRed() const;  ///< Red-version wild encounters.
  int getMonsRedSize() const;                            ///< @see getMonsRedSize property.
  Q_INVOKABLE const MapDBEntryWildMon* getMonsRedAt(const int ind) const; ///< Red wild @p ind (for QML).

  const QVector<MapDBEntryWildMon*> getMonsBlue() const; ///< Blue-version wild encounters.
  int getMonsBlueSize() const;                           ///< @see getMonsBlueSize property.
  Q_INVOKABLE const MapDBEntryWildMon* getMonsBlueAt(const int ind) const; ///< Blue wild @p ind (for QML).

  const QVector<MapDBEntryWildMon*> getMonsWater() const; ///< Water wild encounters.
  int getMonsWaterSize() const;                           ///< @see getMonsWaterSize property.
  Q_INVOKABLE const MapDBEntryWildMon* getMonsWaterAt(const int ind) const; ///< Water wild @p ind (for QML).

  int getSpriteSet() const;             ///< @see getSpriteSet property.
  SpriteSetDBEntry* getToSpriteSet() const; ///< @see getToSpriteSet property.
  int getBorder() const;                ///< @see getBorder property.
  int getBank() const;                  ///< @see getBank property.
  int getDataPtr() const;               ///< @see getDataPtr property.
  int getScriptPtr() const;             ///< @see getScriptPtr property.
  int getTextPtr() const;               ///< @see getTextPtr property.
  int getWidth() const;                 ///< @see getWidth property.
  int getHeight() const;                ///< @see getHeight property.
  const QString getMusic() const;       ///< @see getMusic property.
  const QString getTileset() const;     ///< @see getTileset property.
  const QString getModernName() const;  ///< @see getModernName property.
  const QString getIncomplete() const;  ///< @see getIncomplete property.
  MusicDBEntry* getToMusic() const;     ///< @see getToMusic property.
  TilesetDBEntry* getToTileset() const; ///< @see getToTileset property.
  MapDBEntry* getToComplete() const;    ///< @see getToComplete property.

  const QVector<EventDBEntry*> getToEvents() const; ///< Events associated with this map.
  int getToEventsSize() const;                      ///< @see getToEventsSize property.
  Q_INVOKABLE const EventDBEntry* getToEventsAt(const int ind) const; ///< Associated event @p ind (for QML).

  FlyDBEntry* getToFlyDestination() const; ///< @see getToFlyDestination property.

  const QVector<HiddenItemDBEntry*> getToHiddenItems() const; ///< Hidden items on this map.
  int getToHiddenItemsSize() const;                           ///< @see getToHiddenItemsSize property.
  Q_INVOKABLE const HiddenItemDBEntry* getToHiddenItemsAt(const int ind) const; ///< Hidden item @p ind (for QML).

  ScriptDBEntry* getToScript() const; ///< @see getToScript property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntry();                    ///< Empty entry (built by MapsDB).
  MapDBEntry(const QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                 ///< Resolve the full cross-reference web.
  void qmlRegister() const;        ///< Register with QML.

  // Optional bool values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  QString name = ""; ///< Backing field (read via getName()).
  int ind = -1;      ///< Backing field (read via getInd()).

  bool glitch = false;  ///< Backing field (read via getGlitch()).
  bool special = false; ///< Backing field (read via getSpecial()).

  // Warps to other maps
  QVector<MapDBEntryWarpOut*> warpOut; ///< Outgoing warps.

  // Warps In from other maps
  QVector<MapDBEntryWarpIn*> warpIn;   ///< Incoming warps.

  // Signs on map
  QVector<MapDBEntrySign*> signs;      ///< Signs.

  // The map's text-pointer table (id -> words + category), imported from pret/pokered.
  QVector<MapDBEntryText*> textEntries; ///< Text-pointer table entries (index 0 == text id 1).

  // The map's named script steps (Current-script picker), imported by scripts/import_map_scripts.py.
  QVector<MapScriptStep> scriptSteps; ///< Named script-step list; empty for unscripted maps.

  // Sprites on map
  QVector<MapDBEntrySprite*> sprites;  ///< Sprites.

  // Connecting Maps
  QHash<int, MapDBEntryConnect*> connect; ///< Edge connections by direction.

  // Wild Pokemon Encounter Rate
  // Along with mons for Red & Blue & Water Mons
  // Although there is strangely only 1 map in the game that carries both
  // water and land Pokemon. More strangely there's only 1 set of water Pokemon
  // that all maps share that have water
  int monRate = -1;                      ///< Land encounter rate.
  int monRateWater = -1;                 ///< Water encounter rate.
  QVector<MapDBEntryWildMon*> monsRed;   ///< Red-version land encounters.
  QVector<MapDBEntryWildMon*> monsBlue;  ///< Blue-version land encounters.
  QVector<MapDBEntryWildMon*> monsWater; ///< Water encounters (shared; see note).

  // Sprite Set
  int spriteSet = -1;                     ///< Sprite-set index.
  SpriteSetDBEntry* toSpriteSet = nullptr; ///< Resolved sprite set (deepLink).

  // Border Block #
  int border = -1; ///< Border block number.

  int bank = -1;      ///< Map data bank.
  int dataPtr = -1;   ///< Map data pointer.
  int scriptPtr = -1; ///< Map script pointer.
  int textPtr = -1;   ///< Map text pointer.
  int width = -1;     ///< Width (blocks).
  int height = -1;    ///< Height (blocks).

  QString music = "";      ///< Music name.
  QString tileset = "";    ///< Tileset name.
  QString modernName = ""; ///< Modern display name.
  QString incomplete = ""; ///< Incomplete-map marker.

  // Deep Linking
  MusicDBEntry* toMusic = nullptr; // To Map Music
  TilesetDBEntry* toTileset = nullptr; // To Map Tileset
  MapDBEntry* toComplete = nullptr; // To Complete Version of Map
  QVector<EventDBEntry*> toEvents; // To Associated Events
  FlyDBEntry* toFlyDestination = nullptr; // To Associated Fly Destination
  QVector<HiddenItemDBEntry*> toHiddenItems; // To Associated Hidden Items
  ScriptDBEntry* toScript = nullptr; ///< Resolved script (deepLink).

  friend class MapsDB;
  friend class MapSearch;      // reads filter fields in filter methods
  friend struct EventDBEntry;  // writes toEvents in deepLink
  friend struct FlyDBEntry;    // writes toFlyDestination in deepLink
  friend struct HiddenItemDBEntry; // writes toHiddenItems in deepLink
  friend struct ScriptDBEntry; // writes toScript in deepLink
};
