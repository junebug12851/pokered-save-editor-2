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
#include <QString>
#include <QAbstractListModel>
#include <QVector>

// monFromBox() takes a PokemonBox from QML (PokemonDetails passes its boxData).
// Full include so the parameter is a real, matchable PokemonBox QObject type.
#include <pse-savefile/expanded/fragments/pokemonbox.h>

class PokemonDBEntry;
class PokemonBox;

/// One move picker row: display name + move index.
struct MoveSelectEntry {
  MoveSelectEntry(QString name, int ind);

  QString name; ///< Display name.
  int ind;      ///< Move index.
};

/**
 * @brief Move picker model -- context-aware on a chosen Pokemon.
 *
 * Select-model variant (see SpeciesSelectModel) with a twist: when @ref mon is set
 * (via monFromBox() from the details editor) the list rebuilds to that species'
 * legal moves (rebuildListSpecific); otherwise it shows the general move list.
 * Exposed as `brg.moveSelectModel`.
 */
class MoveSelectModel : public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(PokemonDBEntry* mon MEMBER mon NOTIFY monChanged) ///< Current species context (filters the list).

signals:
  void monChanged();

public:
  /// Picker columns (mapped in roleNames()).
  enum ItemRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  MoveSelectModel();

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

  Q_INVOKABLE int moveToListIndex(int ind); ///< Row index for move @p ind.

  void onMonChange();          ///< React to @ref mon changing.
  void rebuildListGeneral();   ///< Rebuild with all moves.
  void rebuildListSpecific();  ///< Rebuild with @ref mon's legal moves only.

public slots:
  void monFromBox(PokemonBox* box); ///< Set @ref mon from a QML-passed PokemonBox.

public:
  QVector<MoveSelectEntry*> moveListCache; ///< Cached picker rows.
  PokemonDBEntry* mon = nullptr;           ///< @see mon property.
};
