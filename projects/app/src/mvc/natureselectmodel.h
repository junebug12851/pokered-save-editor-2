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
#ifndef NATURESELECTMODEL_H
#define NATURESELECTMODEL_H

#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include <QVector>

struct NatureSelectEntry {
  NatureSelectEntry(QString name, int ind);

  QString name;
  int ind;
};

class NatureSelectModel : public QAbstractListModel
{
  Q_OBJECT

public:
  enum ItemRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  NatureSelectModel();

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;

  QVector<NatureSelectEntry*> natureListCache;

  Q_INVOKABLE int natureToListIndex(int ind);
};

#endif // NATURESELECTMODEL_H
