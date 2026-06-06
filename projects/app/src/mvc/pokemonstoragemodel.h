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

// getBoxMon()/getPartyMon() are Q_INVOKABLE and return these to QML (the Pokémon
// details screen binds them to typed PokemonBox/PokemonParty properties). Full
// includes so QML receives real QObjects, not opaque values.
// See notes/reference/qt6-patterns.md.
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>

class PokemonBox;
class PokemonParty;
class PokemonStorageBox;
class PlayerPokemon;
class Storage;
class Router;

class PokemonStorageModel : public QAbstractListModel
{
  Q_OBJECT

  // Checkmarks changed
  Q_PROPERTY(bool hasChecked READ hasCheckedCached NOTIFY hasCheckedChangedCached STORED false)

  Q_PROPERTY(int curBox MEMBER curBox NOTIFY curBoxChanged)

signals:
  void hasCheckedChanged();
  void hasCheckedChangedCached();
  void curBoxChanged();

public:
  // Name of attached properties
  static constexpr const char* isCheckedKey = "isChecked";

  enum BagItemRoles {
    IndRole = Qt::UserRole + 1,
    DexRole,
    NameRole,
    CheckedRole,
    PlaceholderRole,
    NicknameRole,
    LevelRole,
    IsShinyRole,
    IsPartyRole,
  };

  enum BoxSelect {
    PartyBox = -1
  };

  PokemonStorageModel(
      Router* router,
      Storage* storage,
      PlayerPokemon* party
      );

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;
  bool setData(const QModelIndex &index, const QVariant &value,
                   int role = Qt::EditRole) override;

  QVariant getPlaceHolderData(int role) const;

  // Signals from the box
  void onMove(int from, int to);
  void onRemove(int ind);
  void onInsert();

  // Attached property management
  bool hasChecked();
  bool hasCheckedCached();
  QVector<PokemonBox*> getChecked();
  void onBeforeRelocate(PokemonBox* item);
  void checkStateDirty();

  void pageClosing();

  Q_INVOKABLE PokemonBox* getBoxMon(int index);
  Q_INVOKABLE PokemonParty* getPartyMon(int index);

public slots:
  // Attached property management
  void clearCheckedState();
  void clearCheckedStateGone();
  void onReset();

  void checkedMoveToTop();
  void checkedMoveUp();
  void checkedMoveDown();
  void checkedMoveToBottom();
  void checkedDelete();
  void checkedTransfer();
  void checkedToggleAll();

  void switchBox(int newBox, bool force = false);
  PokemonStorageBox* getCurBox() const;
  PokemonStorageBox* getBox(int box) const;

public:
  int curBox = PartyBox;
  Router* router = nullptr;
  Storage* storage = nullptr;
  PlayerPokemon* party = nullptr;
  PokemonStorageModel* otherModel = nullptr;
  bool checkedStateDirty = false;
};
