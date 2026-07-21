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

/**
 * @file itemmarketviewmodel.cpp
 * @brief Implementation of ItemMarketViewModel -- the Buy/Sell view proxy. See header.
 */

#include "./itemmarketviewmodel.h"
#include "./itemmarketmodel.h"

ItemMarketViewModel::ItemMarketViewModel(ItemMarketModel* source, QObject* parent)
  : QSortFilterProxyModel(parent),
    m_market(source)
{
  setSourceModel(source);
  setDynamicSortFilter(true);

  // Toggling Buy/Sell does NOT rebuild the model (that would clear the cart); it
  // only re-slices the left view, so re-filter when the flag flips.
  connect(source, &ItemMarketModel::isBuyModeChanged,
          this, [this]() { invalidateFilter(); });
}

bool ItemMarketViewModel::filterAcceptsRow(int sourceRow,
                                           const QModelIndex& sourceParent) const
{
  // Exchange mode is a separate, single list -- show it wholesale.
  if(m_market->isExchangeMode)
    return true;

  const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
  if(!idx.isValid())
    return false;

  const int tag = sourceModel()->data(idx, ItemMarketModel::ViewTagRole).toInt();
  const int want = m_market->isBuyMode ? ItemMarketModel::ViewBuy
                                       : ItemMarketModel::ViewSell;
  return tag == want;
}
