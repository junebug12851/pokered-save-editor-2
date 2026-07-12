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

// Children that QML TRAVERSES must be full types here. A Q_DECLARE_OPAQUE_POINTER'd QObject sets
// IsPointerToTypeDerivedFromQObject = false, so QML reads the whole chain past it as `undefined` --
// silently. Everything else stays forward-declared + opaque, because pulling all 11 in drags the
// heavy area subtree (whose .cpp's include db headers) into the global include path and balloons
// build time. See notes/reference/qt-patterns.md.
//
//   general -> the playtime "countPlaytime" helper
//   audio   -> the Map screen's Music panel (musicID / musicBank / the two flags).
//              Being opaque is what made that panel read "No save open" with a save wide open,
//              on 2026-07-12. It is the same trap, for the second time.
#include "./areageneral.h"
#include "./areaaudio.h"

class AreaLoadedSprites;
class AreaMap;
class AreaNPC;
class AreaPlayer;
class AreaPokemon;
class AreaSign;
class AreaSprites;
class AreaTileset;
class AreaWarps;

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

/**
 * @brief The current map's complete live state -- everything about "where you are".
 *
 * Groups the eleven facets of the map the player currently stands on: @ref audio,
 * @ref preloadedSprites, @ref general, @ref map, @ref npc, @ref player position,
 * wild @ref pokemon, @ref signs, @ref sprites, @ref tileset, and @ref warps.
 * Follows the standard expanded-node convention (load/save/reset/randomize -- see
 * SaveFileExpanded), and adds setTo(MapDBEntry*) which is the heart of the
 * "place the player on any map" feature: it reconfigures the whole area to a
 * chosen map with correct warps/sprites/music/tileset.
 *
 * @note Only @ref general is QML-traversed, so only it is a full include here; the
 *       other ten children are kept opaque above purely for build speed (see the
 *       in-code note). This mirrors `savefile_autoport.h`'s opaque list.
 * @see SaveFileExpanded, MapDBEntry, and [the system map](../../../../../notes/systems/savefile.md).
 */
class SAVEFILE_AUTOPORT Area : public QObject
{
  Q_OBJECT

  Q_PROPERTY(AreaAudio* audio MEMBER audio NOTIFY audioChanged)                       ///< Map music/audio settings.
  Q_PROPERTY(AreaLoadedSprites* preloadedSprites MEMBER preloadedSprites NOTIFY preloadedSpritesChanged) ///< Sprite sets loaded for the map.
  Q_PROPERTY(AreaGeneral* general MEMBER general NOTIFY generalChanged)               ///< Contrast, letter delay, playtime toggle (QML-traversed).
  Q_PROPERTY(AreaMap* map MEMBER map NOTIFY mapChanged)                               ///< Map id, size, pointers, connections.
  Q_PROPERTY(AreaNPC* npc MEMBER npc NOTIFY npcChanged)                               ///< NPC movement state.
  Q_PROPERTY(AreaPlayer* player MEMBER player NOTIFY playerChanged)                   ///< Player position/facing on the map.
  Q_PROPERTY(AreaPokemon* pokemon MEMBER pokemon NOTIFY pokemonChanged)               ///< Wild-encounter data.
  Q_PROPERTY(AreaSign* signs MEMBER signs NOTIFY signsChanged)                        ///< The map's signs.
  Q_PROPERTY(AreaSprites* sprites MEMBER sprites NOTIFY spritesChanged)               ///< The map's sprites/NPCs.
  Q_PROPERTY(AreaTileset* tileset MEMBER tileset NOTIFY tilesetChanged)               ///< The map's tileset.
  Q_PROPERTY(AreaWarps* warps MEMBER warps NOTIFY warpsChanged)                       ///< The map's warps.

public:
  Area(SaveFile* saveFile = nullptr);
  virtual ~Area();

  void load(SaveFile* saveFile = nullptr); ///< Expand the whole area from the save.
  void save(SaveFile* saveFile);           ///< Flatten the whole area to the save.

signals:
  // There's actually no need for these, Q_PROPERTY requires them
  void audioChanged();
  void preloadedSpritesChanged();
  void generalChanged();
  void mapChanged();
  void npcChanged();
  void playerChanged();
  void pokemonChanged();
  void signsChanged();
  void spritesChanged();
  void tilesetChanged();
  void warpsChanged();

public slots:
  void reset();              ///< Blank the whole area.
  void randomize();          ///< Randomize the area (constrained, playable).
  void setTo(MapDBEntry* map); ///< Reconfigure the area to @p map (the "place on any map" feature).

public:
  AreaAudio* audio = nullptr;                  ///< @see audio property.
  AreaLoadedSprites* preloadedSprites = nullptr; ///< @see preloadedSprites property.
  AreaGeneral* general = nullptr;              ///< @see general property.
  AreaMap* map = nullptr;                      ///< @see map property.
  AreaNPC* npc = nullptr;                      ///< @see npc property.
  AreaPlayer* player = nullptr;                ///< @see player property.
  AreaPokemon* pokemon = nullptr;              ///< @see pokemon property.
  AreaSign* signs = nullptr;                   ///< @see signs property.
  AreaSprites* sprites = nullptr;              ///< @see sprites property.
  AreaTileset* tileset = nullptr;              ///< @see tileset property.
  AreaWarps* warps = nullptr;                  ///< @see warps property.
};
