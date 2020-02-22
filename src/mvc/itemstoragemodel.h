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

  Q_PROPERTY(bool hasChecked READ hasChecked NOTIFY hasCheckedChanged STORED false)

signals:
  void hasCheckedChanged();

public:
  static constexpr const char* isCheckedKey = "isChecked";

  enum BagItemRoles {
    IdRole = Qt::UserRole + 1,
    CountRole,
    WorthAllRole,
    WorthEachRole,
    CheckedRole
  };

  ItemStorageModel(ItemStorageBox* items, Router* router);

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;
  bool setData(const QModelIndex &index, const QVariant &value,
                   int role = Qt::EditRole) override;

  void onMove(int from, int to);
  void onRemove(int ind);
  void onInsert();
  void onReset();

  bool hasChecked();
  QVector<Item*> getChecked();

public slots:
  void clearCheckedState();
  void clearCheckedStateGone();
  void checkedMoveToTop();
  void checkedMoveUp();
  void checkedMoveDown();
  void checkedMoveToBottom();
  void checkedDelete();
  void checkedTransfer();
  void checkedToggleAll();
  void onBeforeRelocate(Item* item);

public:
  ItemStorageBox* items = nullptr;
  Router* router;
};

#endif // BAGITEMSMODEL_H
