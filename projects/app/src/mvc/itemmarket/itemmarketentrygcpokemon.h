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
#include "./itemmarketentry.h"

class GameCornerDBEntry;
class Storage;
class PlayerPokemon;
class PlayerBasics;

/**
 * @brief Market row for a Game Corner Pokemon prize (bought with coins).
 *
 * An ItemMarketEntry subtype representing a Game Corner Pokemon (@ref toGameCorner)
 * purchasable with coins; checkout() adds the received mon to the party
 * (@ref party) or, if full, PC @ref storage. See ItemMarketEntry.
 */
class ItemMarketEntryGCPokemon : public ItemMarketEntry
{
  Q_OBJECT

public:
  ItemMarketEntryGCPokemon(GameCornerDBEntry* toGameCorner, PlayerPokemon* party, Storage* storage);
  virtual ~ItemMarketEntryGCPokemon();

  virtual QString _name() override;       ///< @copydoc ItemMarketEntry::_name
  virtual int _inStockCount() override;   ///< @copydoc ItemMarketEntry::_inStockCount
  virtual bool _canSell() override;       ///< @copydoc ItemMarketEntry::_canSell
  virtual int _itemWorth() override;      ///< @copydoc ItemMarketEntry::_itemWorth
  virtual QString _whichType() override;  ///< @copydoc ItemMarketEntry::_whichType
  virtual int onCartLeft() override;      ///< @copydoc ItemMarketEntry::onCartLeft
  virtual int stackCount() override;      ///< @copydoc ItemMarketEntry::stackCount

public slots:
  virtual void checkout() override; ///< Buy the prize mon (into party/storage).

public:
  static constexpr const char* type = "gcPokemon"; ///< This row's type key.
  GameCornerDBEntry* toGameCorner = nullptr; ///< The prize definition.
  PlayerPokemon* party = nullptr;            ///< Destination party.
  Storage* storage = nullptr;                ///< Overflow PC storage.
};
