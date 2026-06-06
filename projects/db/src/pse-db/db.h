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

// Calling DB::inst() bootstraps the entire database module:
// all sub-databases are created, loaded, indexed, and deep-linked.
// No DB class should be accessed before DB::inst() is called.
class DB_AUTOPORT DB : public QObject
{
  Q_OBJECT

  Q_PROPERTY(GameData*        json         READ json         CONSTANT)
  Q_PROPERTY(CreditsDB*       credits      READ credits      CONSTANT)
  Q_PROPERTY(EventPokemonDB*  eventPokemon READ eventPokemon CONSTANT)
  Q_PROPERTY(EventsDB*        events       READ events       CONSTANT)
  Q_PROPERTY(Examples*        examples     READ examples     CONSTANT)
  Q_PROPERTY(Names*           names        READ names        CONSTANT)
  Q_PROPERTY(FlyDB*           fly          READ fly          CONSTANT)
  Q_PROPERTY(FontsDB*         fonts        READ fonts        CONSTANT)
  Q_PROPERTY(GameCornerDB*    gameCorner   READ gameCorner   CONSTANT)
  Q_PROPERTY(HiddenCoinsDB*   hiddenCoins  READ hiddenCoins  CONSTANT)
  Q_PROPERTY(HiddenItemsDB*   hiddenItems  READ hiddenItems  CONSTANT)
  Q_PROPERTY(ItemsDB*         items        READ items        CONSTANT)
  Q_PROPERTY(MapsDB*          maps         READ maps         CONSTANT)
  Q_PROPERTY(MissablesDB*     missables    READ missables    CONSTANT)
  Q_PROPERTY(MovesDB*         moves        READ moves        CONSTANT)
  Q_PROPERTY(MusicDB*         music        READ music        CONSTANT)
  Q_PROPERTY(PokemonDB*       pokemon      READ pokemon      CONSTANT)
  Q_PROPERTY(ScriptsDB*       scripts      READ scripts      CONSTANT)
  Q_PROPERTY(SpriteSetDB*     spriteSets   READ spriteSets   CONSTANT)
  Q_PROPERTY(SpritesDB*       sprites      READ sprites      CONSTANT)
  Q_PROPERTY(StarterPokemonDB* starters    READ starters     CONSTANT)
  Q_PROPERTY(TilesetDB*       tilesets     READ tilesets     CONSTANT)
  Q_PROPERTY(TmHmsDB*         tmHms        READ tmHms        CONSTANT)
  Q_PROPERTY(TradesDB*        trades       READ trades       CONSTANT)
  Q_PROPERTY(TrainersDB*      trainers     READ trainers     CONSTANT)
  Q_PROPERTY(TypesDB*         types        READ types        CONSTANT)

public:
  [[nodiscard]] static DB* inst();

  [[nodiscard]] GameData*        json()         const;
  [[nodiscard]] CreditsDB*       credits()      const;
  [[nodiscard]] EventPokemonDB*  eventPokemon() const;
  [[nodiscard]] EventsDB*        events()       const;
  [[nodiscard]] Examples*        examples()     const;
  [[nodiscard]] Names*           names()        const;
  [[nodiscard]] FlyDB*           fly()          const;
  [[nodiscard]] FontsDB*         fonts()        const;
  [[nodiscard]] GameCornerDB*    gameCorner()   const;
  [[nodiscard]] HiddenCoinsDB*   hiddenCoins()  const;
  [[nodiscard]] HiddenItemsDB*   hiddenItems()  const;
  [[nodiscard]] ItemsDB*         items()        const;
  [[nodiscard]] MapsDB*          maps()         const;
  [[nodiscard]] MissablesDB*     missables()    const;
  [[nodiscard]] MovesDB*         moves()        const;
  [[nodiscard]] MusicDB*         music()        const;
  [[nodiscard]] PokemonDB*       pokemon()      const;
  [[nodiscard]] ScriptsDB*       scripts()      const;
  [[nodiscard]] SpriteSetDB*     spriteSets()   const;
  [[nodiscard]] SpritesDB*       sprites()      const;
  [[nodiscard]] StarterPokemonDB* starters()    const;
  [[nodiscard]] TilesetDB*       tilesets()     const;
  [[nodiscard]] TmHmsDB*         tmHms()        const;
  [[nodiscard]] TradesDB*        trades()       const;
  [[nodiscard]] TrainersDB*      trainers()     const;
  [[nodiscard]] TypesDB*         types()        const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;
  void qmlHook(QQmlContext* const context) const;

private slots:
  void initRes()      const;
  void qmlRegister()  const;
  void loadAll()      const;
  void indexAll()     const;
  void deepLinkAll()  const;

private:
  DB();
};
