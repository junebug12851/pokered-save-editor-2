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
 * @file itemmarketcartmodel.cpp
 * @brief Implementation of ItemMarketCartModel -- the cart filter proxy. See the header.
 */

#include "./itemmarketcartmodel.h"
#include "./itemmarketmodel.h"

ItemMarketCartModel::ItemMarketCartModel(ItemMarketModel* source, QObject* parent)
  : QSortFilterProxyModel(parent)
{
  setSourceModel(source);

  // Re-evaluate filterAcceptsRow on every source dataChanged. The market model
  // emits a per-row dataChanged when a cart count is set, and a model-wide
  // dataChanged on each recompute, so a row enters/leaves the receipt the moment
  // its cart count crosses zero -- no manual invalidate needed.
  setDynamicSortFilter(true);
}

bool ItemMarketCartModel::filterAcceptsRow(int sourceRow,
                                           const QModelIndex& sourceParent) const
{
  const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
  if(!idx.isValid())
    return false;

  // Section headers ("msg" rows) never belong on a receipt.
  if(sourceModel()->data(idx, ItemMarketModel::WhichTypeRole).toString() == "msg")
    return false;

  // Keep only rows the user actually put on the cart.
  return sourceModel()->data(idx, ItemMarketModel::CartCountRole).toInt() > 0;
}
