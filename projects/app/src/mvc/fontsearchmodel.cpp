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

#include <pse-db/fonts.h>
#include <pse-db/util/fontsearch.h>
#include "./fontsearchmodel.h"

FontSearchModel::FontSearchModel(FontSearch* search)
{
  this->search = search;
  connect(search, &FontSearch::fontCountChanged, this, &FontSearchModel::onDataChange);
}

int FontSearchModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return search->results.size();
}

QVariant FontSearchModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= search->results.size())
    return QVariant();

  // Return array index to lookup font data
  if (role == IndRole)
    return search->results.at(index.row())->ind;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> FontSearchModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "fontInd";

  return roles;
}

void FontSearchModel::onDataChange()
{
  // I'm not about to code in a way to figure out every little data piece change
  beginResetModel();
  endResetModel();
}
