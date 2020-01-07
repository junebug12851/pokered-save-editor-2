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
#ifndef MAP_H
#define MAP_H

#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>

#include "optional"

#include "../../common/types.h"

struct MusicDBEntry;
struct TilesetDBEntry;

struct MapDBEntry;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Details on all the maps in the game

// Hard-Coded value, this is the value that the gen 1 games use, it refers
// to a ram address that never changes related to the overworld map
constexpr var16 worldMapPtr = 0xC6E8;

enum class ConnectDir : var8
{
  NORTH,
  SOUTH,
  EAST,
  WEST
};

/*
 * A forewarning!!
 *
 * Map Connections are one of the most complicated aspects of the entire game
 * by far and large. Out of any gen 1 data structure, forumlas, algorithms, math
 * etc... Virtually out of anythign within the game code, this is the most
 * complicated part. Anyone I've talked to and most docs I've read have all
 * been in agreement. It's also one of the most difficult to get right and the
 * most difficult to understand.
 *
 * If anything is slightly wrong, the game will liekly crash quickly or at the
 * least not work right. So... yea... that's why it's inclusion is so late in
 * this app and the reason why I've been so hesitant to bring in this
 * functionality.
 *
 * Sources to help me:
 *
 * PokeRed Team
 * https://github.com/pret/pokered/blob/master/macros/data_macros.asm
 *
 * Bulbapedia User Tiddlywinks
 * https://bulbapedia.bulbagarden.net/wiki/User:Tiddlywinks/Map_header_data_structure_in_Generation_I
*/
struct MapDBEntryConnect {

  MapDBEntryConnect();
  MapDBEntryConnect(ConnectDir dir, MapDBEntry* fromMap, QJsonValue& data);

  void deepLink();

  // Direction used in calculating
  ConnectDir dir;

  // Connecting Map
  QString map = "";

  // Connecting Strip Centering
  svar8 stripMove = 0;

  // Connecting Strip Position
  svar8 stripOffset = 0;

  // Offset strip by an additional 3 for unknown
  // reasons
  bool flag = false;

  // To connecting map
  MapDBEntry* toMap = nullptr;

  // Map with connection
  MapDBEntry* fromMap = nullptr;

  // Location of strip
  // Pointer to start in connected map
  var16 stripLocation();

  // Map Position
  // Pointer to start of connection
  var16 mapPos();

  // Strip Size
  // Connection size (blocks)
  var8 stripSize();

  // Player Pos
  // Player Y & X Offset (steps)
  svar8 yAlign();
  svar8 xAlign();

  // Map VRAM Offset
  // Pointer to window
  var16 window();
};

// List of Warps on Map that warp out to a different map
// They can only warp to a "warp-in" point
struct MapDBEntryWarpOut
{
  void deepLink();

  // X & Y location on Map
  var8 x;
  var8 y;

  // Which pre-defined warp-in to warp to
  var8 warp;

  // Which map to warp to
  QString map;

  // Is this warp-out not intended to be used
  bool glitch = false;

  // Go to map
  MapDBEntry* toMap = nullptr;
};

struct MapDBEntry {

  // Optional bool values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  MapDBEntry();

  QString name;
  var8 ind;

  bool glitch;
  bool special;

  // Warps to other maps
  QVector<MapDBEntryWarpOut*> warpOut;

  // Connecting Maps
  QHash<var8,MapDBEntryConnect*> connect;

  // Border Block #
  std::optional<var8> border;

  std::optional<var8> bank;
  std::optional<var16> dataPtr;
  std::optional<var16> scriptPtr;
  std::optional<var16> textPtr;
  std::optional<var8> width;
  std::optional<var8> height;

  QString music = "";
  QString tileset = "";
  QString modernName = "";
  QString incomplete = "";

  // These have been removed from the JSON data because they are simply
  // dimensions times 2 and thus redundant and repetitive to inlclude in JSON
  std::optional<var8> height2X2();
  std::optional<var8> width2X2();

  // Deep Linking
  MusicDBEntry* toMusic = nullptr; // To Map Music
  TilesetDBEntry* toTileset = nullptr; // To Map Tileset
  MapDBEntry* toComplete = nullptr; // To Complete Version of Map
};

class MapsDB
{
public:
  static void load();
  static void index();
  static void deepLink();

  static QVector<MapDBEntry*> store;
  static QHash<QString, MapDBEntry*> ind;
};

#endif // MAP_H
