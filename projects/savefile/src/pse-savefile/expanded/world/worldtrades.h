/*
  * Copyright 2020 Fairy Fox
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
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

constexpr var8 tradeCount = 10;    ///< In-game NPC trades tracked.
constexpr var8 tradeByteCount = 2; ///< 6 of 16 bits unused

/**
 * @brief "Done" flags for the game's in-game (NPC) trades.
 *
 * A @ref completedTrades bool array of @ref tradeCount flags marking which of the
 * one-time NPC trades have already been completed. QML count/at/set access.
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see World.
 */
class SAVEFILE_AUTOPORT WorldTrades : public QObject
{
  Q_OBJECT

public:
  WorldTrades(SaveFile* saveFile = nullptr);
  virtual ~WorldTrades();

  void load(SaveFile* saveFile = nullptr); ///< Expand the trade flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten the trade flags to the save.

  Q_INVOKABLE int tradesCount();              ///< Number of trade flags (tradeCount).
  Q_INVOKABLE bool tradesAt(int ind);         ///< Is trade @p ind completed?
  Q_INVOKABLE void tradesSet(int ind, bool val); ///< Set/clear completed for trade @p ind.

signals:
  void completedTradesChanged();

public slots:
  void reset();     ///< Clear all trade flags.
  void randomize(); ///< Randomize the trade flags.

public:
  bool completedTrades[tradeCount]; ///< Per-trade completed flags.
};
