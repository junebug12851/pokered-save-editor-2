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

#include "./itemmarketentrymessage.h"


ItemMarketEntryMessage::ItemMarketEntryMessage(QString msg)
  : msg(msg)
{
  finishConstruction();
  exclude = true;
}

ItemMarketEntryMessage::~ItemMarketEntryMessage() {}

QString ItemMarketEntryMessage::_name()
{
  return msg;
}

int ItemMarketEntryMessage::_inStockCount()
{
  return 0;
}

bool ItemMarketEntryMessage::_canSell()
{
  return false;
}

int ItemMarketEntryMessage::_itemWorth()
{
  return 0;
}

QString ItemMarketEntryMessage::_whichType()
{
  return type;
}

int ItemMarketEntryMessage::onCartLeft()
{
  return 0;
}

int ItemMarketEntryMessage::stackCount()
{
  return 0;
}

void ItemMarketEntryMessage::checkout() {}