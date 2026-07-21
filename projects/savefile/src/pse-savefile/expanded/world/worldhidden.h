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

// There's actually significantly more hidden item bits, 112 in total forming
// 14 bytes. But given they are all unused it makes no sense to load entire
// unused bytes
constexpr var8 hiddenItemCount = 54;       ///< Hidden-item collected flags actually used.
constexpr var8 hiddenCoinCount = 12;       ///< Hidden-coin collected flags actually used.
constexpr var8 hiddenItemByteCount = 7;    ///< 2 Bits of 56 unused
constexpr var8 hiddenCoinByteCount = 2;    ///< 4 Bits of 16 unused

/**
 * @brief "Already collected" flags for hidden items and hidden Game Corner coins.
 *
 * Two bool arrays (@ref hiddenItems, @ref hiddenCoins) tracking which hidden
 * pickups you've already grabbed. Only the @e used bits are loaded (the save has
 * more, but they're unused -- see the count note above). QML count/at/set access.
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see World.
 */
class SAVEFILE_AUTOPORT WorldHidden : public QObject
{
  Q_OBJECT

public:
  WorldHidden(SaveFile* saveFile = nullptr);
  virtual ~WorldHidden();

  void load(SaveFile* saveFile = nullptr); ///< Expand the hidden-item/coin flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten the hidden-item/coin flags to the save.

  Q_INVOKABLE int hItemsCount();              ///< Number of hidden-item flags.
  Q_INVOKABLE bool hItemsAt(int ind);         ///< Is hidden item @p ind collected?
  Q_INVOKABLE void hItemsSet(int ind, bool val); ///< Set/clear hidden item @p ind.

  Q_INVOKABLE int hCoinsCount();              ///< Number of hidden-coin flags.
  Q_INVOKABLE bool hCoinsAt(int ind);         ///< Is hidden coin @p ind collected?
  Q_INVOKABLE void hCoinsSet(int ind, bool val); ///< Set/clear hidden coin @p ind.

signals:
  void hiddenItemsChanged();
  void hiddenCoinsChanged();

public slots:
  void reset();     ///< Clear all hidden flags.
  void randomize(); ///< Randomize the hidden flags.

public:
  bool hiddenItems[hiddenItemCount]; ///< Hidden-item collected flags.
  bool hiddenCoins[hiddenCoinCount]; ///< Hidden-coin collected flags.
};
