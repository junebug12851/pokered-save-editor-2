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

/**
 * @file creditsmodel.cpp
 * @brief Implementation of CreditsModel. See creditsmodel.h.
 */

#include "./creditsmodel.h"
#include <QVariantMap>
#include <pse-db/creditsdb.h>
#include <pse-db/entries/creditdbentry.h>

void CreditsModel::ensureBuilt() const
{
  if (m_built)
    return;

  // Walk the flat store: a non-empty section name starts a new group; every
  // other entry is an item under the current group.
  int cur = -1;
  for (auto entry : CreditsDB::inst()->getStore())
  {
    if (!entry->getSection().isEmpty())
    {
      m_sections.append(Section{ entry->getSection(), {} });
      cur = m_sections.size() - 1;
      continue;
    }

    if (cur < 0)
      continue; // defensive: an entry before any header

    QVariantMap item;
    item["name"]     = entry->getName();
    item["url"]      = entry->getUrl();
    item["note"]     = entry->getNote();
    item["license"]  = entry->getLicense();
    item["mandated"] = entry->getMandated();
    m_sections[cur].entries.append(item);
  }

  m_built = true;
}

int CreditsModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent)

  ensureBuilt();
  return m_sections.size();
}

QVariant CreditsModel::data(const QModelIndex& index, int role) const
{
  ensureBuilt();

  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() < 0 || index.row() >= m_sections.size())
    return QVariant();

  const Section& section = m_sections.at(index.row());

  if (role == SectionRole)
    return section.name;

  if (role == EntriesRole)
    return section.entries;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> CreditsModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[SectionRole] = "section";
  roles[EntriesRole] = "entries";

  return roles;
}
