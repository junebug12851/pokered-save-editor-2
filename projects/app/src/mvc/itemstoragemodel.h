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
#include <QString>
#include <QAbstractListModel>
#include <QVector>

class Item;
class ItemStorageBox;
class Router;

/**
 * @brief Editable list model for an item box (the bag or a PC item box).
 *
 * Wraps an ItemStorageBox (see CreditsModel for the base convention) as an editable
 * model: rows can be reordered/removed/inserted, and each row carries a checkbox
 * state (a QML attached property, @ref isCheckedKey). The `checked*` slots act on
 * the checked rows in bulk -- move up/down/top/bottom, delete, transfer (to the
 * paired box), toggle-all. @ref hasChecked drives "any selected" UI. Exposed as
 * `brg.bagItemsModel` / `brg.pcItemsModel`.
 *
 * @see ItemStorageBox, Item.
 */
class ItemStorageModel : public QAbstractListModel
{
  Q_OBJECT

  // Checkmarks changed
  Q_PROPERTY(bool hasChecked READ hasCheckedCached NOTIFY hasCheckedChangedCached STORED false) ///< Are any rows checked?

signals:
  void hasCheckedChanged();
  void hasCheckedChangedCached();

public:
  // Name of attached properties
  static constexpr const char* isCheckedKey = "isChecked"; ///< QML attached-property name for the per-row checkbox.

  /// Columns (mapped in roleNames()).
  enum BagItemRoles {
    IdRole = Qt::UserRole + 1,
    CountRole,
    CheckedRole,
    PlaceholderRole,
  };

  ItemStorageModel(ItemStorageBox* items, Router* router); ///< @param items the box; @param router for page hooks.

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.
  bool setData(const QModelIndex &index, const QVariant &value,
                   int role = Qt::EditRole) override;                       ///< Edit a row (e.g. checkbox).

  QVariant getPlaceHolderData(int role) const; ///< The "empty slot" placeholder row's data.

  // Signals from the box
  void onMove(int from, int to); ///< React to a box move.
  void onRemove(int ind);        ///< React to a box removal.
  void onInsert();               ///< React to a box insert.
  void onReset();                ///< React to a box reset.

  // Attached property management
  bool hasChecked();              ///< Are any rows checked (live)?
  bool hasCheckedCached();        ///< Cached form (backs the property).
  QVector<Item*> getChecked();    ///< The currently-checked items.
  void onBeforeRelocate(Item* item); ///< Cleanup hook before an item relocates away.
  void checkStateDirty();         ///< Mark the checked-state cache stale.

  void pageClosing(); ///< Hook for when the page closes.

  // ---- Drag & drop (Bag / Items screen) -----------------------------------
  // Backing for the list's drag-to-reorder and drag-between-panes gestures, the
  // direct analogue of PokemonStorageModel's. `toIndex` is the destination
  // insertion slot (0..count): the moved item(s) land *before* the item at that
  // slot (== append when toIndex is the count). When `group` is true the whole
  // currently-checked set moves together (preserving order); otherwise just the
  // single item at `fromIndex`. Same rules as the checked* bulk actions
  // (destination never overflows). An item box can be empty, so -- unlike the
  // Pokemon party -- there is no "never empties" guard.

  /// Reorder within this box: move @p fromIndex (or the checked set) to @p toIndex.
  Q_INVOKABLE void dragReorder(int fromIndex, int toIndex, bool group);

  /// Move @p fromIndex (or the checked set) from this box into the paired box,
  /// inserting at @p toIndex there.
  Q_INVOKABLE void dragTransfer(int fromIndex, int toIndex, bool group);

  /// Delete the item at @p index, or -- when @p group -- the whole checked set
  /// (the per-row hover/checked delete button; replaces the old footer delete).
  Q_INVOKABLE void deleteItem(int index, bool group);

public slots:
  // Attached property management
  void clearCheckedState();     ///< Uncheck everything.
  void clearCheckedStateGone(); ///< Clear checked state for removed rows.

  void checkedMoveToTop();    ///< Move checked rows to the top.
  void checkedMoveUp();       ///< Move checked rows up one.
  void checkedMoveDown();     ///< Move checked rows down one.
  void checkedMoveToBottom(); ///< Move checked rows to the bottom.
  void checkedDelete();       ///< Delete checked rows.
  void checkedTransfer();     ///< Transfer checked rows to the paired box.
  void checkedToggleAll();    ///< Toggle all checkboxes.

public:
  ItemStorageBox* items = nullptr; ///< The wrapped item box.
  Router* router;                  ///< For page hooks.
  ItemStorageModel* otherModel = nullptr; ///< The paired sibling model (for cross-pane transfers).
  bool checkedStateDirty = false;  ///< Whether the checked-state cache needs refresh.
};
