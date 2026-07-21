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
#include <QAbstractListModel>

class FileManagement;

/**
 * @brief Recent-files list model for the start screen.
 *
 * Wraps FileManagement's recent-files list (see CreditsModel for the convention),
 * shortening each path for display and tracking enabled/index per row. Exposed as
 * `brg.recentFilesModel`.
 */
class RecentFilesModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Columns (mapped in roleNames()).
  enum RecentFileRoles {
    PathRole = Qt::UserRole + 1,
    EnabledRole,
    FileIndexRole,
  };

  RecentFilesModel(FileManagement* file); ///< @param file the file controller.

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

private:
  QString getDisplayPath(int index, QString path) const; ///< Shorten a path for display.
  void onDataChange();             ///< React to the recent-files list changing.
  FileManagement* file = nullptr;  ///< The file controller.
};
