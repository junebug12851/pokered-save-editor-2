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
#include <QVector>
#include <QObject>
#include <QString>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class PokemonBox;
struct PokemonDBEntry;

/**
 * @brief The eight Gym badges, in bit order, as a QML-visible enum.
 *
 * A QObject purely so the `Badges_` enum is exposed to QML (registered creatable
 * in bootQmlLinkage). The values are the badge bit indices within the badge byte.
 */
struct SAVEFILE_AUTOPORT Badges : public QObject
{
  Q_OBJECT
  Q_ENUMS(Badges_)

public:
  enum Badges_ : var8 {
    Boulder = 0, ///< Boulder Badge (Brock).
    Cascade,     ///< Cascade Badge (Misty).
    Thunder,     ///< Thunder Badge (Lt. Surge).
    Rainbow,     ///< Rainbow Badge (Erika).
    Soul,        ///< Soul Badge (Koga).
    Marsh,       ///< Marsh Badge (Sabrina).
    Volcano,     ///< Volcano Badge (Blaine).
    Earth        ///< Earth Badge (Giovanni).
  };
};

constexpr var8 maxBadges = 8; ///< Number of Gym badges.

/**
 * @brief The trainer's headline values: name, ID, money, coins, badges, starter.
 *
 * A leaf-heavy expanded node (see SaveFileExpanded for the load/save/reset/
 * randomize convention). Beyond the simple money/coins values it carries two
 * pieces of cross-cutting logic:
 * - **Full-set name/ID** (see the property note below): editing the trainer's
 *   name or ID also rewrites every non-traded Pokemon's OT name/ID so they stay
 *   "owned by you".
 * - **Badges** as a bool[8], with QML-facing count/at/set helpers.
 *
 * @see Player (owns this), PokemonBox (the non-trade mons it keeps in sync).
 */
class SAVEFILE_AUTOPORT PlayerBasics : public QObject
{
  Q_OBJECT

  // We use the full-set versions of playername and ID so that all QML changes
  // will properly update non-trade mons and keep the non-trade status. Otherwise
  // the user will have to manually go through and change all of the non-trade mons
  // OT Name and OT ID after changing it here which is undesirable.
  Q_PROPERTY(QString playerName READ getPlayerName WRITE fullSetPlayerName NOTIFY playerNameChanged) ///< Trainer name; writing it full-sets non-trade mons (see note above).
  Q_PROPERTY(int playerID READ getPlayerId WRITE fullSetPlayerId NOTIFY playerIDChanged) ///< Trainer ID; writing it full-sets non-trade mons.
  Q_PROPERTY(unsigned int money MEMBER money NOTIFY moneyChanged) ///< Money (24-bit BCD on disk, max 999,999).
  Q_PROPERTY(int coins MEMBER coins NOTIFY coinsChanged)         ///< Casino coins (16-bit BCD on disk, max 9,999).
  Q_PROPERTY(int playerStarter MEMBER playerStarter NOTIFY playerStarterChanged) ///< Chosen starter (drives rival logic).

public:
  PlayerBasics(SaveFile* saveFile = nullptr);
  virtual ~PlayerBasics();

  void load(SaveFile* saveFile = nullptr); ///< Expand the trainer basics from the save.
  void save(SaveFile* saveFile);           ///< Flatten the trainer basics back to the save.
  void setBadges(SaveFile* saveFile, var16 offset); ///< Write the badge bits at @p offset.

  PokemonDBEntry* toStarter(); ///< Resolve @ref playerStarter to its DB entry.

  Q_INVOKABLE int badgeCount();            ///< How many badges are set.
  Q_INVOKABLE bool badgeAt(int ind);       ///< Is badge @p ind set?
  Q_INVOKABLE void badgeSet(int ind, bool val); ///< Set/clear badge @p ind.

  QString getPlayerName(); ///< Current trainer name (backs the property READ).
  int getPlayerId();       ///< Current trainer ID (backs the property READ).

  QVector<PokemonBox*> getNonTradeMons();        ///< All party/box mons whose OT is this trainer (not traded in).
  void fixNonTradeMons(QVector<PokemonBox*> mons); ///< Rewrite those mons' OT name/ID to match this trainer.

signals:
  void playerNameChanged();
  void playerIDChanged();
  void moneyChanged();
  void coinsChanged();
  void badgesChanged();
  void playerStarterChanged();

public slots:
  void reset();     ///< Blank the trainer basics.
  void randomize(); ///< Randomize the trainer basics (constrained).

  // Full Set also goes through the Pokemon and ensures non-traded pokemon are
  // properly updated
  void fullSetPlayerName(QString val); ///< Set name AND fix non-trade mons' OT name.
  void fullSetPlayerId(int id);        ///< Set ID AND fix non-trade mons' OT ID.

  void randomizeStarter(); ///< Randomize just the starter.
  void randomizeCoins();   ///< Randomize just the coin count.
  void randomizeMoney();   ///< Randomize just the money.
  void randomizeID();      ///< Randomize just the trainer ID.

public:
  QString playerName;        ///< Trainer name (backs the property).
  int playerID;              ///< Trainer ID (backs the property).
  unsigned int money;        ///< Money value.
  int coins;                 ///< Casino coins.
  bool badges[maxBadges] = {
    false, // Boulder
    false, // Cascade
    false, // Thunder
    false, // Rainbow
    false, // Soul
    false, // Marsh
    false, // Volcano
    false // Earth
  };                         ///< Per-badge owned flags, indexed by Badges::Badges_.
  int playerStarter;         ///< Chosen starter species.

  SaveFile* file = nullptr;  ///< Owning save (used by the full-set helpers to reach the mons).
};
