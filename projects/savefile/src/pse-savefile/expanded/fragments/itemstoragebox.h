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
class Item;

/**
 * @brief A container of Items -- either the trainer's bag or a PC item box.
 *
 * Holds a vector of @ref items up to @ref maxSize. The @ref isBag flag chooses bag
 * vs PC behaviour (and drives which randomizer runs). Exposes live counts and
 * total worth to QML, supports add/remove/move/sort, and can relocate items to its
 * paired box (bag <-> PC) via @ref destBox.
 *
 * @see Item (slot type), Player (owns the bag), Storage (owns the PC box).
 */
class SAVEFILE_AUTOPORT ItemStorageBox : public QObject
{
  Q_OBJECT

  /// How many items are there.
  Q_PROPERTY(int itemsCount READ itemsCount NOTIFY itemsChanged STORED false)

  /// How many items including item amounts are there.
  Q_PROPERTY(int itemsCountBulk READ itemsCountBulk NOTIFY itemsChanged STORED false)

  /// Maximum number of items.
  Q_PROPERTY(int itemsMax READ itemsMax STORED false CONSTANT)

  /// Is this the players bag or PC storage.
  Q_PROPERTY(bool isBag READ getIsBag CONSTANT)

  /// Can we relocate? If the other box is full then we cannot.
  Q_PROPERTY(bool relocateFull READ relocateFull NOTIFY itemsChanged STORED false)

  /// What's the worth of all the items?
  Q_PROPERTY(int itemsAllBuyMoney READ itemsAllBuyMoney NOTIFY itemsChanged STORED false)   ///< Total buy value in money.
  Q_PROPERTY(int itemsAllBuyCoins READ itemsAllBuyCoins NOTIFY itemsChanged STORED false)   ///< Total buy value in coins.
  Q_PROPERTY(int itemsAllSellMoney READ itemsAllSellMoney NOTIFY itemsChanged STORED false) ///< Total sell value in money.
  Q_PROPERTY(int itemsAllSellCoins READ itemsAllSellCoins NOTIFY itemsChanged STORED false) ///< Total sell value in coins.

  /// Get destination box (the paired bag/PC box for relocation).
  Q_PROPERTY(ItemStorageBox* destBox READ destBox STORED false CONSTANT)

public:
  /// @param isBag bag vs PC; @param maxSize capacity; @param offset box location in the save.
  ItemStorageBox(bool isBag, int maxSize, SaveFile* saveFile = nullptr, int offset = 0);
  virtual ~ItemStorageBox();

  void load(SaveFile* saveFile = nullptr, int offset = 0); ///< Expand the box from the save.
  void save(SaveFile* saveFile, int offset);               ///< Flatten the box to the save.

  // These are provided as properties
  int itemsCount();      ///< Distinct item count.
  int itemsCountBulk();  ///< Item count including stack amounts.
  int itemsMax();        ///< Capacity.
  bool getIsBag();       ///< Is this the bag?
  bool relocateFull();   ///< Is relocation blocked because the paired box is full?
  ItemStorageBox* destBox(); ///< The paired box for relocation.

  int itemsAllBuyMoney();  ///< @see itemsAllBuyMoney property.
  int itemsAllBuyCoins();  ///< @see itemsAllBuyCoins property.
  int itemsAllSellMoney(); ///< @see itemsAllSellMoney property.
  int itemsAllSellCoins(); ///< @see itemsAllSellCoins property.

  // Methods that can be called from QML which cannot be properties and make no
  // sense as slots
  Q_INVOKABLE Item* itemAt(int ind); ///< Item slot @p ind (GC-protected return).

  // Auto-called from randomize function depending on box type
  void randomizeStorage(); ///< Randomizer path for a PC item box.
  void randomizeBag();     ///< Randomizer path for the bag.

signals:
  // When moving an item away from the box to another box, allow the model to
  // perform cleanup actions
  void beforeItemRelocate(Item* item); ///< Emitted before a relocate so models can clean up.

  // Alert the model on changes
  void itemsChanged();             ///< Box contents changed.
  void itemMoveChange(int from, int to); ///< An item moved slot.
  void itemRemoveChange(int ind);  ///< An item was removed.
  void itemInsertChange();         ///< An item was inserted.
  void itemsResetChange();         ///< The box was reset.

public slots:

  // These alow the model or QML to interact with the box
  void reset();                 ///< Empty the box.
  void randomize();             ///< Randomize (dispatches to bag/storage path).
  bool itemMove(int from, int to); ///< Reorder an item.
  void itemRemove(int ind);     ///< Remove item @p ind.
  void itemNew();               ///< Add a fresh item.
  bool relocateAll();           ///< Move every item to the paired box.
  bool relocateOne(int ind);    ///< Move one item to the paired box.
  void sort();                  ///< Sort the box contents.

public:
  QVector<Item*> items; ///< The stored items.

  // These are set from the creating class SAVEFILE_AUTOPORT indicating details of the box
  // It's crucial they are read-only
  int maxSize;    ///< Capacity (set at construction; treat as read-only).
  bool isBag;     ///< Bag vs PC (set at construction; treat as read-only).
  SaveFile* file; ///< Owning save.
};
