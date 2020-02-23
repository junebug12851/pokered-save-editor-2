/*
  * Copyright 2020 June Hanabi
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
#ifndef BAGITEMSMODEL_H
#define BAGITEMSMODEL_H

#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include <QVector>

class Item;
class ItemStorageBox;
class Router;

class ItemStorageModel : public QAbstractListModel
{
  Q_OBJECT

  // Checkmarks changed
  Q_PROPERTY(bool hasChecked READ hasCheckedCached NOTIFY hasCheckedChangedCached STORED false)

signals:
  void hasCheckedChanged();
  void hasCheckedChangedCached();

public:
  // Name of attached properties
  static constexpr const char* isCheckedKey = "isChecked";

  enum BagItemRoles {
    IdRole = Qt::UserRole + 1,
    CountRole,
    WorthAllRole,
    WorthEachRole,
    CheckedRole,
    PlaceholderRole,
  };

  ItemStorageModel(ItemStorageBox* items, Router* router);

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;
  bool setData(const QModelIndex &index, const QVariant &value,
                   int role = Qt::EditRole) override;

  QVariant getPlaceHolderData(int role) const;

  // Signals from the box
  void onMove(int from, int to);
  void onRemove(int ind);
  void onInsert();
  void onReset();

  // Attached property management
  bool hasChecked();
  bool hasCheckedCached();
  QVector<Item*> getChecked();
  void onBeforeRelocate(Item* item);
  void checkStateDirty();

  void pageClosing();

public slots:
  // Attached property management
  void clearCheckedState();
  void clearCheckedStateGone();

  void checkedMoveToTop();
  void checkedMoveUp();
  void checkedMoveDown();
  void checkedMoveToBottom();
  void checkedDelete();
  void checkedTransfer();
  void checkedToggleAll();

public:
  ItemStorageBox* items = nullptr;
  Router* router;
  bool checkedStateDirty = false;
};

#endif // BAGITEMSMODEL_H
