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

// ── EVERY child of Area is a FULL TYPE here, and none of them is opaque ──────────────────────
//
// A Q_DECLARE_OPAQUE_POINTER'd QObject sets IsPointerToTypeDerivedFromQObject = false, so QML
// reads the whole chain past it as `undefined` -- silently, with no warning and a green
// tst_qml_screens. That trap has now bitten this project three times: the original
// dataExpanded.* chain (session 13), the Music panel reading "No save open" with a save wide
// open (AreaAudio, 2026-07-12), and -- had we not fixed it here -- every single field of the
// new Map screen, which edits the whole Area block from QML.
//
// The nine that used to be opaque (map, player, tileset, warps, signs, sprites, pokemon, npc,
// preloadedSprites) were kept that way purely for BUILD SPEED. That trade is off: the Map screen
// traverses all of them (notes/plans/map-screen.md), so they are full includes now. If the build
// slows measurably, the fix is to trim the sub-tree's own includes -- NOT to make a QML-traversed
// type opaque again.
//
// See notes/reference/qt-patterns.md. tst_qml_brg (test_areaChildrenQmlTraverses_resolve) is the
// net that catches a regression here.
#include "./areageneral.h"
#include "./areaaudio.h"
#include "./arealoadedsprites.h"
#include "./areamap.h"
#include "./areanpc.h"
#include "./areaplayer.h"
#include "./areapokemon.h"
#include "./areasign.h"
#include "./areasprites.h"
#include "./areatileset.h"
#include "./areawarps.h"

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
 * @note **All eleven children are full types here and NONE is opaque** (2026-07-12). The Map screen
 *       edits the whole Area block from QML, and a `Q_DECLARE_OPAQUE_POINTER`'d QObject makes QML
 *       read the entire chain past it as `undefined`, silently. Never re-opaque a QML-traversed
 *       type -- `tst_qml_brg` (`test_areaChildrenQmlTraverses_resolve`) is what catches you.
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
  void setTo(MapDBEntry* map); ///< Reconfigure the area to @p map (the "place on any map" feature; random arrival point).
  void setTo(MapDBEntry* map, int x, int y); ///< Same, landing the player at (@p x, @p y) — the deterministic map-states path.

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
