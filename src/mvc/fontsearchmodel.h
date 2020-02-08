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
#ifndef FONTSEARCHMODEL_H
#define FONTSEARCHMODEL_H

#include <QObject>
#include <QAbstractListModel>

class FontSearch;

class FontSearchModel : public QAbstractListModel
{
  Q_OBJECT

public:
  enum FontSearchRoles {
    IndRole = Qt::UserRole + 1,
  };

  FontSearchModel(FontSearch* search);

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;

private:
  void onDataChange();
  FontSearch* search = nullptr;
};

#endif // FONTSEARCHMODEL_H
