/*
  * Copyright 2026 Twilight
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

/**
 * @file itemoverviewmodel.cpp
 * @brief Implementation of ItemOverviewModel. See itemoverviewmodel.h.
 */

#include <algorithm>
#include <QHash>
#include <QSet>
#include <QCollator>

#include "./itemoverviewmodel.h"
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-db/entries/itemdbentry.h>

ItemOverviewModel::ItemOverviewModel(ItemStorageBox* bag, ItemStorageBox* storage)
  : bag(bag),
    storage(storage)
{
  // Stay live: any add/remove/move/amount-change on either box re-aggregates.
  connect(this->bag, &ItemStorageBox::itemsChanged, this, &ItemOverviewModel::rebuild);
  connect(this->storage, &ItemStorageBox::itemsChanged, this, &ItemOverviewModel::rebuild);

  rebuild();
}

int ItemOverviewModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent)
  return rows.size();
}

QVariant ItemOverviewModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid() || index.row() < 0 || index.row() >= rows.size())
    return QVariant();

  const Row& r = rows.at(index.row());

  if(role == NameRole)
    return r.name;
  else if(role == BagCountRole)
    return r.bag;
  else if(role == StorageCountRole)
    return r.storage;

  return QVariant();
}

QHash<int, QByteArray> ItemOverviewModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[NameRole] = "itemName";
  roles[BagCountRole] = "bagCount";
  roles[StorageCountRole] = "storageCount";
  return roles;
}

void ItemOverviewModel::rebuild()
{
  beginResetModel();
  rows.clear();

  // Aggregate by item index, summing amounts across any duplicate rows (the
  // editor supports a box holding the same item more than once). Iterate the
  // BOXES (not the items DB) so glitch/unknown items present in the save still
  // appear; resolve each name from the item itself.
  QHash<int, int> bagAmt;
  QHash<int, int> stoAmt;
  QHash<int, QString> names;

  auto collect = [&names](ItemStorageBox* box, QHash<int, int>& amt) {
    if(box == nullptr)
      return;
    for(auto item : box->items) {
      if(item == nullptr)
        continue;
      int ind = item->ind;
      amt[ind] += item->amount;
      if(!names.contains(ind)) {
        auto entry = item->toItem();
        names[ind] = (entry != nullptr) ? entry->getReadable() : QStringLiteral("???");
      }
    }
  };

  collect(bag, bagAmt);
  collect(storage, stoAmt);

  // Union of all item indices seen in either box.
  QSet<int> inds;
  for(auto it = bagAmt.constBegin(); it != bagAmt.constEnd(); ++it)
    inds.insert(it.key());
  for(auto it = stoAmt.constBegin(); it != stoAmt.constEnd(); ++it)
    inds.insert(it.key());

  for(int ind : inds) {
    int b = bagAmt.value(ind, 0);
    int s = stoAmt.value(ind, 0);

    // Drop items that are effectively absent everywhere (e.g. a stray 0-amount
    // row) -- a row only earns its place if the user actually holds some.
    if(b <= 0 && s <= 0)
      continue;

    rows.append(Row{ names.value(ind, QStringLiteral("???")), b, s });
  }

  // Alphabetical, mirroring ItemStorageBox::sort (numeric-aware, punctuation-
  // insensitive) so the table reads the same way regardless of either box's order.
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);
  std::sort(rows.begin(), rows.end(), [&collator](const Row& a, const Row& b) {
    return collator.compare(a.name, b.name) < 0;
  });

  endResetModel();
}
