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
#ifndef MAPDBENTRYCONNECT_H
#define MAPDBENTRYCONNECT_H

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
struct DB_AUTOPORT MapDBEntryConnect : public QObject {
  Q_OBJECT
  Q_PROPERTY(int stripLocation READ stripLocation CONSTANT)
  Q_PROPERTY(int mapPos READ mapPos CONSTANT)
  Q_PROPERTY(int stripSize READ stripSize CONSTANT)
  Q_PROPERTY(int yAlign READ yAlign CONSTANT)
  Q_PROPERTY(int xAlign READ xAlign CONSTANT)
  Q_PROPERTY(int window READ window CONSTANT)
  Q_PROPERTY(ConnectDir getDir READ getDir CONSTANT)
  Q_PROPERTY(QString getMap READ getMap CONSTANT)
  Q_PROPERTY(int getStripMove READ getStripMove CONSTANT)
  Q_PROPERTY(int getStripOffset READ getStripOffset CONSTANT)
  Q_PROPERTY(bool getFlag READ getFlag CONSTANT)
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT)
  Q_PROPERTY(MapDBEntry* getFromMap READ getFromMap CONSTANT)
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT)

public:
  // Hard-Coded value, this is the value that the gen 1 games use, it refers
  // to a ram address that never changes related to the overworld map
  static const constexpr int worldMapPtr = 0xC6E8;

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
  int stripLocation() const;

  // Map Position
  // Pointer to start of connection
  // AKA Strip Dst
  int mapPos() const;

  // Strip Size
  // Connection size (blocks)
  // AKA stripWidth
  int stripSize() const;

  // Player Pos
  // Player Y & X Offset (steps)
  // AKA X-Align/Y-Align
  int yAlign() const;
  int xAlign() const;

  // Map VRAM Offset
  // Pointer to window
  // AKA => viewPtr & UL Corner
  int window() const;

  ConnectDir getDir() const;
  const QString getMap() const;
  int getStripMove() const;
  int getStripOffset() const;
  bool getFlag() const;
  const MapDBEntry* getToMap() const;
  const MapDBEntry* getFromMap() const;
  const MapDBEntry* getParent() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  MapDBEntryConnect();
  MapDBEntryConnect(const ConnectDir dir,
                    MapDBEntry* const fromMap,
                    const QJsonValue& data);
  void deepLink();
  void qmlRegister() const;

  // Direction used in calculating
  ConnectDir dir = ConnectDir::NORTH;

  // Connecting Map
  QString map = "";

  // Connecting Strip Centering
  int stripMove = 0;

  // Connecting Strip Position
  int stripOffset = 0;

  // Offset strip by an additional 3 for unknown
  // reasons
  bool flag = false;

  // To connecting map
  MapDBEntry* toMap = nullptr;

  // Map with connection
  MapDBEntry* fromMap = nullptr;
  MapDBEntry* parent = nullptr; // Basically the same thing, just an alias

  friend class MapDBEntry;
};

#endif // MAPDBENTRYCONNECT_H
