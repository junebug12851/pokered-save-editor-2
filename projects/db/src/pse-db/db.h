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
#include <QQmlContext>
#include "./db_autoport.h"

// Qt 6 requires Q_PROPERTY pointer types to be fully defined (not just
// forward-declared) so MOC can register them as metatypes. Include all
// sub-database headers here rather than forward-declaring them.
#include "./util/gamedata.h"
#include "./blocksdb.h"
#include "./creditsdb.h"
#include "./eventpokemondb.h"
#include "./eventsdb.h"
#include "./examples.h"
#include "./names.h"
#include "./flydb.h"
#include "./fontsdb.h"
#include "./gamecornerdb.h"
#include "./hiddencoinsdb.h"
#include "./hiddenItemsdb.h"
#include "./itemsdb.h"
#include "./mapsdb.h"
#include "./missablesdb.h"
#include "./moves.h"
#include "./music.h"
#include "./pokemon.h"
#include "./scripts.h"
#include "./spriteSet.h"
#include "./sprites.h"
#include "./starterPokemon.h"
#include "./tileset.h"
#include "./tmHm.h"
#include "./trades.h"
#include "./trainers.h"
#include "./types.h"

class QQmlEngine;

/**
 * @brief The aggregate of every game-data database -- the single entry point that
 *        boots the whole `db` layer.
 *
 * `db` holds the game's static reference data (every Pokemon, move, item, map,
 * font glyph, trainer, ...), loaded from JSON assets, independent of any save
 * file. This class gathers all 26 sub-databases under one QObject and exposes each
 * as a `Q_PROPERTY`, so QML reaches them as `db.pokemon`, `db.moves`, etc.
 *
 * @par Bootstrap (the critical sequence)
 * Calling DB::inst() bootstraps the entire database module: all sub-databases are
 * created, then loadAll() -> indexAll() -> deepLinkAll() run in order
 * (deep-linking resolves cross-references, e.g. a move's type, an evolution's
 * target). @b No DB class should be accessed before DB::inst() is called.
 *
 * @note The constructors of the sub-DBs must NOT call load() themselves -- that
 *       caused a Qt 6 static-init deadlock; loading is centralised here. See
 *       [decisions/architecture.md](../../../../notes/decisions/architecture.md).
 * @see [the db system map](../../../../notes/systems/db.md), GameData (the JSON source).
 */
class DB_AUTOPORT DB : public QObject
{
  Q_OBJECT

  Q_PROPERTY(GameData*        json         READ json         CONSTANT) ///< Raw parsed JSON assets behind every DB.
  Q_PROPERTY(BlocksDB*        blocks       READ blocks       CONSTANT) ///< Raw map block layouts + tileset blocksets.
  Q_PROPERTY(CreditsDB*       credits      READ credits      CONSTANT) ///< Credits/attribution entries.
  Q_PROPERTY(EventPokemonDB*  eventPokemon READ eventPokemon CONSTANT) ///< Event/gift Pokemon definitions.
  Q_PROPERTY(EventsDB*        events       READ events       CONSTANT) ///< Story-event metadata.
  Q_PROPERTY(Examples*        examples     READ examples     CONSTANT) ///< Example player/pokemon/rival presets.
  Q_PROPERTY(Names*           names        READ names        CONSTANT) ///< Default player/pokemon name lists.
  Q_PROPERTY(FlyDB*           fly          READ fly          CONSTANT) ///< Fly destinations.
  Q_PROPERTY(FontsDB*         fonts        READ fonts        CONSTANT) ///< In-game font glyphs / text encoding.
  Q_PROPERTY(GameCornerDB*    gameCorner   READ gameCorner   CONSTANT) ///< Game Corner prize data.
  Q_PROPERTY(HiddenCoinsDB*   hiddenCoins  READ hiddenCoins  CONSTANT) ///< Hidden-coin definitions.
  Q_PROPERTY(HiddenItemsDB*   hiddenItems  READ hiddenItems  CONSTANT) ///< Hidden-item definitions.
  Q_PROPERTY(ItemsDB*         items        READ items        CONSTANT) ///< All items (with prices).
  Q_PROPERTY(MapsDB*          maps         READ maps         CONSTANT) ///< Every map and its layout/connections.
  Q_PROPERTY(MissablesDB*     missables    READ missables    CONSTANT) ///< Missable-sprite definitions.
  Q_PROPERTY(MovesDB*         moves        READ moves        CONSTANT) ///< All moves (type, PP, ...).
  Q_PROPERTY(MusicDB*         music        READ music        CONSTANT) ///< Music tracks.
  Q_PROPERTY(PokemonDB*       pokemon      READ pokemon      CONSTANT) ///< All 151 species and their data.
  Q_PROPERTY(ScriptsDB*       scripts      READ scripts      CONSTANT) ///< Map script metadata.
  Q_PROPERTY(SpriteSetDB*     spriteSets   READ spriteSets   CONSTANT) ///< Sprite-set definitions.
  Q_PROPERTY(SpritesDB*       sprites      READ sprites      CONSTANT) ///< Individual sprite definitions.
  Q_PROPERTY(StarterPokemonDB* starters    READ starters     CONSTANT) ///< Valid starter species.
  Q_PROPERTY(TilesetDB*       tilesets     READ tilesets     CONSTANT) ///< Tileset definitions.
  Q_PROPERTY(TmHmsDB*         tmHms        READ tmHms        CONSTANT) ///< TM/HM mappings.
  Q_PROPERTY(TradesDB*        trades       READ trades       CONSTANT) ///< In-game trade definitions.
  Q_PROPERTY(TrainersDB*      trainers     READ trainers     CONSTANT) ///< Trainer rosters.
  Q_PROPERTY(TypesDB*         types        READ types        CONSTANT) ///< Type chart / type names.

public:
  /// Bootstraps and returns the process-wide DB (creates + loads + indexes + deep-links).
  [[nodiscard]] static DB* inst();

  [[nodiscard]] GameData*        json()         const; ///< The GameData JSON source (backs @c json).
  [[nodiscard]] BlocksDB*        blocks()       const; ///< The block database (backs @c blocks).
  [[nodiscard]] CreditsDB*       credits()      const; ///< The credits database (backs @c credits).
  [[nodiscard]] EventPokemonDB*  eventPokemon() const; ///< The event-Pokemon database (backs @c eventPokemon).
  [[nodiscard]] EventsDB*        events()       const; ///< The events database (backs @c events).
  [[nodiscard]] Examples*        examples()     const; ///< The examples database (backs @c examples).
  [[nodiscard]] Names*           names()        const; ///< The names database (backs @c names).
  [[nodiscard]] FlyDB*           fly()          const; ///< The fly-destinations database (backs @c fly).
  [[nodiscard]] FontsDB*         fonts()        const; ///< The fonts database (backs @c fonts).
  [[nodiscard]] GameCornerDB*    gameCorner()   const; ///< The Game Corner database (backs @c gameCorner).
  [[nodiscard]] HiddenCoinsDB*   hiddenCoins()  const; ///< The hidden-coins database (backs @c hiddenCoins).
  [[nodiscard]] HiddenItemsDB*   hiddenItems()  const; ///< The hidden-items database (backs @c hiddenItems).
  [[nodiscard]] ItemsDB*         items()        const; ///< The items database (backs @c items).
  [[nodiscard]] MapsDB*          maps()         const; ///< The maps database (backs @c maps).
  [[nodiscard]] MissablesDB*     missables()    const; ///< The missables database (backs @c missables).
  [[nodiscard]] MovesDB*         moves()        const; ///< The moves database (backs @c moves).
  [[nodiscard]] MusicDB*         music()        const; ///< The music database (backs @c music).
  [[nodiscard]] PokemonDB*       pokemon()      const; ///< The Pokemon database (backs @c pokemon).
  [[nodiscard]] ScriptsDB*       scripts()      const; ///< The scripts database (backs @c scripts).
  [[nodiscard]] SpriteSetDB*     spriteSets()   const; ///< The sprite-sets database (backs @c spriteSets).
  [[nodiscard]] SpritesDB*       sprites()      const; ///< The sprites database (backs @c sprites).
  [[nodiscard]] StarterPokemonDB* starters()    const; ///< The starters database (backs @c starters).
  [[nodiscard]] TilesetDB*       tilesets()     const; ///< The tilesets database (backs @c tilesets).
  [[nodiscard]] TmHmsDB*         tmHms()        const; ///< The TM/HM database (backs @c tmHms).
  [[nodiscard]] TradesDB*        trades()       const; ///< The trades database (backs @c trades).
  [[nodiscard]] TrainersDB*      trainers()     const; ///< The trainers database (backs @c trainers).
  [[nodiscard]] TypesDB*         types()        const; ///< The types database (backs @c types).

public slots:
  /// Pin the DB aggregate (and every sub-DB) to C++ ownership so QML never GCs them.
  void qmlProtect(const QQmlEngine* const engine) const;
  /// Install this DB into a QML context (exposes the `db` root).
  void qmlHook(QQmlContext* const context) const;

private slots:
  void initRes()      const; ///< Initialise embedded resources before loading.
  void qmlRegister()  const; ///< Register every DB type with the QML type system.
  void loadAll()      const; ///< Load every sub-database from its JSON assets.
  void indexAll()     const; ///< Build per-DB lookup indexes (by key).
  void deepLinkAll()  const; ///< Resolve cross-database references (the deep-link pass).

private:
  /// Private -- use inst(). Runs the full bootstrap sequence.
  DB();
};
