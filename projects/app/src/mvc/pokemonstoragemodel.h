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

/**
 * @brief Editable list model for a PC box (or the party), with checkbox selection.
 *
 * The Pokemon analogue of ItemStorageModel: shows the mons in the @ref curBox box
 * (or the party when curBox == @ref PartyBox), with per-row checkbox state and the
 * same bulk `checked*` operations (move/delete/transfer/toggle). switchBox() changes
 * which box is shown; getBoxMon()/getPartyMon() hand a typed mon to the details
 * screen. Pairs with a sibling model (@ref otherModel) and a box selector. Exposed
 * as `brg.pokemonStorageModel1/2`.
 *
 * @see Storage, PlayerPokemon, PokemonBoxSelectModel.
 */
class PokemonStorageModel : public QAbstractListModel
{
  Q_OBJECT

  // Checkmarks changed
  Q_PROPERTY(bool hasChecked READ hasCheckedCached NOTIFY hasCheckedChangedCached STORED false) ///< Are any rows checked?

  Q_PROPERTY(int curBox MEMBER curBox NOTIFY curBoxChanged) ///< Currently-shown box (PartyBox = the party).

signals:
  void hasCheckedChanged();
  void hasCheckedChangedCached();
  void curBoxChanged();

public:
  // Name of attached properties
  static constexpr const char* isCheckedKey = "isChecked"; ///< QML attached-property name for the per-row checkbox.

  /// Columns (mapped in roleNames()).
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

  /// Sentinel box index for the party.
  enum BoxSelect {
    PartyBox = -1
  };

  PokemonStorageModel(
      Router* router,
      Storage* storage,
      PlayerPokemon* party
      );

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count of the current box.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.
  bool setData(const QModelIndex &index, const QVariant &value,
                   int role = Qt::EditRole) override;                       ///< Edit a row (e.g. checkbox).

  QVariant getPlaceHolderData(int role) const; ///< The empty-slot placeholder row data.

  // Signals from the box
  void onMove(int from, int to); ///< React to a move.
  void onRemove(int ind);        ///< React to a removal.
  void onInsert();               ///< React to an insert.

  // Attached property management
  bool hasChecked();              ///< Are any rows checked (live)?
  bool hasCheckedCached();        ///< Cached form (backs the property).
  QVector<PokemonBox*> getChecked(); ///< The currently-checked mons.
  void onBeforeRelocate(PokemonBox* item); ///< Cleanup hook before a mon relocates away.
  void checkStateDirty();         ///< Mark the checked-state cache stale.

  void pageClosing(); ///< Hook for when the page closes.

  Q_INVOKABLE PokemonBox* getBoxMon(int index);    ///< Typed box mon at @p index (for the details screen).
  Q_INVOKABLE PokemonParty* getPartyMon(int index); ///< Typed party mon at @p index (for the details screen).

public slots:
  // Attached property management
  void clearCheckedState();     ///< Uncheck everything.
  void clearCheckedStateGone(); ///< Clear checked state for removed rows.
  void onReset();               ///< React to a reset.

  void checkedMoveToTop();    ///< Move checked mons to the top.
  void checkedMoveUp();       ///< Move checked mons up one.
  void checkedMoveDown();     ///< Move checked mons down one.
  void checkedMoveToBottom(); ///< Move checked mons to the bottom.
  void checkedDelete();       ///< Delete checked mons.
  void checkedTransfer();     ///< Transfer checked mons to the paired box.
  void checkedToggleAll();    ///< Toggle all checkboxes.

  void switchBox(int newBox, bool force = false); ///< Show box @p newBox.
  PokemonStorageBox* getCurBox() const;           ///< The currently-shown box object.
  PokemonStorageBox* getBox(int box) const;       ///< The box object for index @p box.

public:
  int curBox = PartyBox;                ///< @see curBox property.
  Router* router = nullptr;             ///< For page hooks.
  Storage* storage = nullptr;           ///< The PC storage.
  PlayerPokemon* party = nullptr;       ///< The party.
  PokemonStorageModel* otherModel = nullptr; ///< The paired sibling model (for transfers).
  bool checkedStateDirty = false;       ///< Whether the checked-state cache needs refresh.
};
