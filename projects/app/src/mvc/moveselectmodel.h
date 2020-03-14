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
#ifndef MOVESELECTMODEL_H
#define MOVESELECTMODEL_H

#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include <QVector>

class PokemonDBEntry;
class PokemonBox;

struct MoveSelectEntry {
  MoveSelectEntry(QString name, int ind);

  QString name;
  int ind;
};

class MoveSelectModel : public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(PokemonDBEntry* mon MEMBER mon NOTIFY monChanged)

signals:
  void monChanged();

public:
  enum ItemRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  MoveSelectModel();

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE int moveToListIndex(int ind);

  void onMonChange();
  void rebuildListGeneral();
  void rebuildListSpecific();

public slots:
  void monFromBox(PokemonBox* box);

public:
  QVector<MoveSelectEntry*> moveListCache;
  PokemonDBEntry* mon = nullptr;
};

#endif // MOVESELECTMODEL_H
