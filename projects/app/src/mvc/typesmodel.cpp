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

#include <pse-db/types.h>
#include "./typesmodel.h"

int TypesModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return TypesDB::store.size() + 1;
}

QVariant TypesModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= TypesDB::store.size() + 1)
    return QVariant();

  // Placeholder for no type
  if(index.row() == 0) {
    if (role == IndRole)
      return 0xFF;
    else if (role == NameRole) {
      return "-----";
    }
  }

  // Get Pokemon and ensure it's valid to prevent crashing
  auto type = TypesDB::store.at(index.row() - 1);
  if(type == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IndRole)
    return type->ind;
  else if (role == NameRole) {
    if(type->readable != "")
      return type->readable;

    return type->name;
  }

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> TypesModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "typeInd";
  roles[NameRole] = "typeName";

  return roles;
}

int TypesModel::valToIndex(int val)
{
  if(val == 0xFF)
    return 0;

  int ret = 0;

  for(int i = 0; i < TypesDB::store.size(); i++) {
    if(TypesDB::store.at(i)->ind == val) {
      ret = i+1;
      break;
    }
  }

  return ret;
}
