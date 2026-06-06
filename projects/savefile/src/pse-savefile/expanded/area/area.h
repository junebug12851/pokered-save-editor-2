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
#include "../../savefile_autoport.h"

class SaveFile;
class Player;
class AreaAudio;
class AreaLoadedSprites;
class AreaGeneral;
class AreaMap;
class AreaNPC;
class AreaPlayer;
class AreaPokemon;
class AreaSign;
class AreaSprites;
class AreaTileset;
class AreaWarps;

// Only `area.general` is traversed in QML (the playtime "countPlaytime" helper),
// so include ONLY that child as a full type. The other 10 area children stay
// forward-declared + Q_DECLARE_OPAQUE_POINTER below. Pulling all 11 in dragged
// the heavy area subtree (whose .cpp's include db headers) into the global
// include path and ballooned build time. See notes/reference/qt6-patterns.md.
#include "./areageneral.h"

class AreaAudio;
class AreaLoadedSprites;
class AreaMap;
class AreaNPC;
class AreaPlayer;
class AreaPokemon;
class AreaSign;
class AreaSprites;
class AreaTileset;
class AreaWarps;

Q_DECLARE_OPAQUE_POINTER(AreaAudio*)
Q_DECLARE_OPAQUE_POINTER(AreaLoadedSprites*)
Q_DECLARE_OPAQUE_POINTER(AreaMap*)
Q_DECLARE_OPAQUE_POINTER(AreaNPC*)
Q_DECLARE_OPAQUE_POINTER(AreaPlayer*)
Q_DECLARE_OPAQUE_POINTER(AreaPokemon*)
Q_DECLARE_OPAQUE_POINTER(AreaSign*)
Q_DECLARE_OPAQUE_POINTER(AreaSprites*)
Q_DECLARE_OPAQUE_POINTER(AreaTileset*)
Q_DECLARE_OPAQUE_POINTER(AreaWarps*)

class MapDBEntry;

class SAVEFILE_AUTOPORT Area : public QObject
{
  Q_OBJECT

  Q_PROPERTY(AreaAudio* audio MEMBER audio NOTIFY audioChanged)
  Q_PROPERTY(AreaLoadedSprites* preloadedSprites MEMBER preloadedSprites NOTIFY preloadedSpritesChanged)
  Q_PROPERTY(AreaGeneral* general MEMBER general NOTIFY generalChanged)
  Q_PROPERTY(AreaMap* map MEMBER map NOTIFY mapChanged)
  Q_PROPERTY(AreaNPC* npc MEMBER npc NOTIFY npcChanged)
  Q_PROPERTY(AreaPlayer* player MEMBER player NOTIFY playerChanged)
  Q_PROPERTY(AreaPokemon* pokemon MEMBER pokemon NOTIFY pokemonChanged)
  Q_PROPERTY(AreaSign* signs MEMBER signs NOTIFY signsChanged)
  Q_PROPERTY(AreaSprites* sprites MEMBER sprites NOTIFY spritesChanged)
  Q_PROPERTY(AreaTileset* tileset MEMBER tileset NOTIFY tilesetChanged)
  Q_PROPERTY(AreaWarps* warps MEMBER warps NOTIFY warpsChanged)

public:
  Area(SaveFile* saveFile = nullptr);
  virtual ~Area();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  // There's actually no need for these, Q_PROPERTY requires them
  void audioChanged();
  void preloadedSpritesChanged();
  void generalChanged();
  void mapChanged();
  void npcChanged();
  void playerChanged();
  void po