/*
  * Copyright 2020 Twilight
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

class FontSearch;

/**
 * @brief Exposes a FontSearch's current results as a list model for the keyboard.
 *
 * Wraps a FontSearch (see CreditsModel for the convention); as the search is
 * filtered, the model refreshes so the on-screen keyboard's grid updates. Exposed
 * as `brg.fontSearchModel`.
 *
 * @see FontSearch.
 */
class FontSearchModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Columns (mapped in roleNames()).
  enum FontSearchRoles {
    IndRole = Qt::UserRole + 1,
  };

  FontSearchModel(FontSearch* search); ///< @param search the finder to mirror.

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

private:
  void onDataChange();          ///< React to the search results changing.
  FontSearch* search = nullptr; ///< The mirrored finder.
};
