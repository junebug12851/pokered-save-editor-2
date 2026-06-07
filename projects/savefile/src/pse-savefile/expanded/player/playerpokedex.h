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
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

constexpr var8 maxPokedex = 151; ///< Number of Gen 1 species (dex slots).

/**
 * @brief The player's Pokedex: a seen flag and an owned flag per species.
 *
 * Two parallel 151-entry bool arrays (@ref seen, @ref owned), expanded from /
 * flattened to the save's two dex bit-fields. Exposes live counts to QML and a
 * full set of QML-callable toggles. Follows the standard expanded-node convention
 * (see SaveFileExpanded).
 *
 * @note Indices are 0-based here; dex number N is index N-1.
 */
class SAVEFILE_AUTOPORT PlayerPokedex : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int ownedCount READ ownedCount NOTIFY dexChanged STORED false) ///< Live count of owned species.
  Q_PROPERTY(int seenCount READ seenCount NOTIFY dexChanged STORED false)   ///< Live count of seen species.

public:
  /// Tri-state of a single dex entry (derived from the seen/owned bits).
  enum DexEntryState {
    DexNone = 0,  ///< Neither seen nor owned.
    DexSeen = 1,  ///< Seen but not owned.
    DexOwned = 2  ///< Owned (implies seen).
  };

  PlayerPokedex(SaveFile* saveFile = nullptr);
  virtual ~PlayerPokedex();

  void load(SaveFile* saveFile = nullptr); ///< Expand both dex bit-fields from the save.
  void save(SaveFile* saveFile);           ///< Flatten both dex bit-fields back to the save.

  /// Expand one dex bit-field (@p fromOffset) into a bool array.
  void loadPokedex(SaveFile* saveFile, QVector<bool>* toArr, var16 fromOffset);
  /// Flatten one bool array back into a dex bit-field (@p toOffset).
  void savePokedex(SaveFile* saveFile, QVector<bool>* fromArr, var16 toOffset);

  int ownedCount(); ///< Count of set @ref owned flags (backs the property).
  int seenCount();  ///< Count of set @ref seen flags (backs the property).

  Q_INVOKABLE int ownedMax();          ///< Max owned index (maxPokedex).
  Q_INVOKABLE bool ownedAt(int ind);   ///< Is species @p ind owned?
  Q_INVOKABLE void ownedSet(int ind, bool val); ///< Set/clear owned for @p ind.

  Q_INVOKABLE int seenMax();           ///< Max seen index (maxPokedex).
  Q_INVOKABLE bool seenAt(int ind);    ///< Is species @p ind seen?
  Q_INVOKABLE void seenSet(int ind, bool val);  ///< Set/clear seen for @p ind.

  Q_INVOKABLE int getState(int ind);   ///< Combined DexEntryState for @p ind.

signals:
  void dexChanged();            ///< Any dex change (refreshes counts).
  void dexItemChanged(int ind); ///< A single entry @p ind changed.

public slots:
  void reset();           ///< Blank the whole dex.
  void randomize();       ///< Randomize the dex (constrained).
  void toggleAll();       ///< Flip every entry.
  void toggleOne(int val); ///< Flip a single entry @p val.
  void markAll(int val);  ///< Mark every entry to a state (e.g. all owned/seen/none).

public:
  bool owned[maxPokedex]; ///< Owned flag per species (0-based).
  bool seen[maxPokedex];  ///< Seen flag per species (0-based).
};
