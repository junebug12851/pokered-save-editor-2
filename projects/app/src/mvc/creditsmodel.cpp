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

#include "./creditsmodel.h"
#include <pse-db/credits.h>

int CreditsModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent)

  return CreditsDB::store.size();
}

QVariant CreditsModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= CreditsDB::store.size())
    return QVariant();

  auto entry = CreditsDB::store.at(index.row());

  if (role == SectionRole)
    return entry->section;

  if (role == NameRole)
    return entry->name;

  if (role == UrlRole)
    return entry->url;

  if (role == NoteRole)
    return entry->note;

  if (role == LicenseRole)
    return entry->license;

  if (role == MandatedRole)
    return entry->mandated;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> CreditsModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[SectionRole] = "section";
  roles[NameRole] = "creditTo";
  roles[UrlRole] = "urlTo";
  roles[NoteRole] = "note";
  roles[LicenseRole] = "license";
  roles[MandatedRole] = "mandated";

  return roles;
}
