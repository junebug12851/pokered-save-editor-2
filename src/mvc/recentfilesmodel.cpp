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

#include <QDir>
#include <QString>
#include <QStringList>

#include "./recentfilesmodel.h"
#include "../data/file/filemanagement.h"

RecentFilesModel::RecentFilesModel(FileManagement* file) : file(file)
{
  connect(file, &FileManagement::recentFilesChanged, this, &RecentFilesModel::onDataChange);
}

int RecentFilesModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return file->recentFilesCount() + 1;
}

QVariant RecentFilesModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= (file->recentFilesCount() + 1))
    return QVariant();

  // If it's the special clear button then treat that specially
  if(index.row() == 0 && role == PathRole)
    return "Clear Recent Files";
  else if(index.row() == 0 && role == EnabledRole)
    return file->recentFilesCount() > 0;
  else if(index.row() == 0 && role == FileIndexRole)
    return -1;

  // else return recent file, actual recent files are always enabled
  if (role == PathRole)
    return getDisplayPath(index.row() - 1, file->getRecentFile(index.row() - 1));

  if (role == EnabledRole)
    return true;

  if(role == FileIndexRole)
    return index.row() - 1;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> RecentFilesModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[EnabledRole] = "isEnabled";
  roles[PathRole] = "path";
  roles[FileIndexRole] = "fileIndex";

  return roles;
}

QString RecentFilesModel::getDisplayPath(int index, QString path) const
{
  auto parts = path.split("/", QString::SkipEmptyParts);
  QString display = "[" + QString::number(index) + "] ";

  if(parts.size() >= 3)
    display += parts.at(0) + "/..."
        + parts.at(parts.size() - 2) + "/"
        + parts.at(parts.size() - 1);
  else if(parts.size() == 2)
    display += parts.at(parts.size() - 2) + "/"
        + parts.at(parts.size() - 1);
  else
    display += parts.at(parts.size() - 1);

  return QDir::toNativeSeparators(display);
}

void RecentFilesModel::onDataChange()
{
  // I'm just trying to say refresh everything, wow this is complicated
  // None of my data handpicks which exact bits and pieces have changed because
  // with the small datasets I have it'd be more expensive and error prone to
  // do that than to just say everything has changed
  // EDIT: It's 5 Rows, Literally no more than 5 entries!
  beginResetModel();
  endResetModel();
}
