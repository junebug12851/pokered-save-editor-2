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
#include <QString>

#include "./itemmarketentry.h"

// The simplest type, just prints out a message

/**
 * @brief Market row that just shows a message (e.g. an empty/placeholder notice).
 *
 * The simplest ItemMarketEntry subtype: it carries a @ref msg and is non-purchasable
 * (its overrides return the no-op values). See ItemMarketEntry.
 */
class ItemMarketEntryMessage : public ItemMarketEntry
{
  Q_OBJECT

public:
  ItemMarketEntryMessage(QString msg); ///< @param msg the message to display.
  virtual ~ItemMarketEntryMessage();

  virtual QString _name() override;       ///< Returns @ref msg.
  virtual int _inStockCount() override;   ///< @copydoc ItemMarketEntry::_inStockCount
  virtual bool _canSell() override;       ///< @copydoc ItemMarketEntry::_canSell
  virtual int _itemWorth() override;      ///< @copydoc ItemMarketEntry::_itemWorth
  virtual QString _whichType() override;  ///< @copydoc ItemMarketEntry::_whichType
  virtual int onCartLeft() override;      ///< @copydoc ItemMarketEntry::onCartLeft
  virtual int stackCount() override;      ///< @copydoc ItemMarketEntry::stackCount

public slots:
  virtual void checkout() override; ///< No-op (messages don't transact).

public:
  QString msg;                                 ///< The displayed message.
  static constexpr const char* type = "msg";   ///< This row's type key.
};
