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
#include <QJsonValue>
#include "../db_autoport.h"

class MapDBEntry;
class QQmlEngine;
class MapDBEntry;

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
/**
 * @brief One edge connection of a map (the seam to a neighbouring map).
 *
 * The DB counterpart to the save's MapConnData -- and, per the forewarning above,
 * the single most complicated data structure in Gen 1. The pointer/offset fields
 * (@ref stripLocation, @ref mapPos, @ref stripSize, @ref yAlign / @ref xAlign,
 * @ref window) are the raw connection math; their AKA names are noted inline. The
 * @ref ConnectDir says which edge. deepLink() resolves @ref toMap. See db.md.
 *
 * @see MapDBEntry (parent), MapConnData (the save-side connection).
 */
struct DB_AUTOPORT MapDBEntryConnect : public QObject {
  Q_OBJECT
  Q_PROPERTY(int stripLocation READ stripLocation CONSTANT) ///< Strip source pointer.
  Q_PROPERTY(int mapPos READ mapPos CONSTANT)               ///< Strip destination pointer.
  Q_PROPERTY(int stripSize READ stripSize CONSTANT)         ///< Connection size (blocks).
  Q_PROPERTY(int yAlign READ yAlign CONSTANT)               ///< Player Y offset (steps).
  Q_PROPERTY(int xAlign READ xAlign CONSTANT)               ///< Player X offset (steps).
  Q_PROPERTY(int window READ window CONSTANT)               ///< View/UL-corner pointer.
  Q_PROPERTY(ConnectDir getDir READ getDir CONSTANT)        ///< Which edge this connects.
  Q_PROPERTY(QString getMap READ getMap CONSTANT)           ///< Connected map name.
  Q_PROPERTY(int getStripMove READ getStripMove CONSTANT)   ///< Strip centering.
  Q_PROPERTY(int getStripOffset READ getStripOffset CONSTANT) ///< Strip position.
  Q_PROPERTY(bool getFlag READ getFlag CONSTANT)            ///< The +3 offset flag (see field note).
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT)   ///< Resolved connected map.
  Q_PROPERTY(MapDBEntry* getFromMap READ getFromMap CONSTANT) ///< Map owning this connection.
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT) ///< Same as fromMap (alias).

public:
  // Hard-Coded value, this is the value that the gen 1 games use, it refers
  // to a ram address that never changes related to the overworld map
  static const constexpr int worldMapPtr = 0xC6E8; ///< Fixed overworld-map RAM pointer (see note).

  /// The four connectable edges.
  enum ConnectDir
  {
    NORTH,
    SOUTH,
    EAST,
    WEST
  };
  Q_ENUM(ConnectDir)

  // Location of strip
  // Pointer to start in connected map
  // AKA Strip Source
  int stripLocation() const; ///< @see stripLocation property (AKA Strip Source).

  // Map Position
  // Pointer to start of connection
  // AKA Strip Dst
  int mapPos() const; ///< @see mapPos property (AKA Strip Dst).

  // Strip Size
  // Connection size (blocks)
  // AKA stripWidth
  int stripSize() const; ///< @see stripSize property (AKA stripWidth).

  // Player Pos
  // Player Y & X Offset (steps)
  // AKA X-Align/Y-Align
  int yAlign() const; ///< @see yAlign property.
  int xAlign() const; ///< @see xAlign property.

  // Map VRAM Offset
  // Pointer to window
  // AKA => viewPtr & UL Corner
  int window() const; ///< @see window property.

  ConnectDir getDir() const;     ///< @see getDir property.
  const QString getMap() const;  ///< @see getMap property.
  int getStripMove() const;      ///< @see getStripMove property.
  int getStripOffset() const;    ///< @see getStripOffset property.
  bool getFlag() const;          ///< @see getFlag property.
  MapDBEntry* getToMap() const;  ///< @see getToMap property.
  MapDBEntry* getFromMap() const; ///< @see getFromMap property.
  MapDBEntry* getParent() const; ///< @see getParent property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntryConnect(); ///< Empty entry.
  MapDBEntryConnect(const ConnectDir dir,
                    MapDBEntry* const fromMap,
                    const QJsonValue& data); ///< Build a @p dir connection on @p fromMap from JSON.
  void deepLink();          ///< Resolve the connected map.
  void qmlRegister() const; ///< Register with QML.

  // Direction used in calculating
  ConnectDir dir = ConnectDir::NORTH; ///< Edge direction.

  // Connecting Map
  QString map = ""; ///< Connected map name.

  // Connecting Strip Centering
  int stripMove = 0; ///< Strip centering.

  // Connecting Strip Position
  int stripOffset = 0; ///< Strip position.

  // Offset strip by an additional 3 for unknown
  // reasons
  bool flag = false; ///< +3 offset flag (purpose unknown; see note).

  // To connecting map
  MapDBEntry* toMap = nullptr; ///< Resolved connected map (deepLink).

  // Map with connection
  MapDBEntry* fromMap = nullptr; ///< Owning map.
  MapDBEntry* parent = nullptr; // Basically the same thing, just an alias

  friend class MapDBEntry;
};
