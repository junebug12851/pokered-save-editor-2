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
#include <QSortFilterProxyModel>

class ItemMarketModel;

/**
 * @brief The "cart" view of @ref ItemMarketModel -- only the rows actually on the cart.
 *
 * A thin filter proxy over the full market model that keeps just the rows the user
 * has put on the cart (`cartCount > 0`) and drops the section-header ("msg") rows.
 * It inherits the source's role names verbatim, so a QML delegate reads exactly the
 * same `data*` roles (`dataName`, `dataCartCount`, `dataItemWorth`, `dataCartWorth`,
 * `dataWhichType`). Exposed as `brg.marketCartModel`; it backs the Pokemart screen's
 * right-hand **receipt** pane, which itemizes the cart instead of summing it.
 *
 * It re-filters live: the source emits per-row dataChanged on a cart edit (setData)
 * plus a model-wide dataChanged on every recompute (onReUpdateValues), and resets on
 * a mode/save change -- with dynamicSortFilter on, rows appear/leave the receipt as
 * their cart count crosses zero. The model-wide totals (total, leftover, space/money
 * checks) stay on @ref ItemMarketModel; the receipt reads those directly.
 *
 * @see ItemMarketModel.
 */
class ItemMarketCartModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  explicit ItemMarketCartModel(ItemMarketModel* source, QObject* parent = nullptr);

protected:
  /// Accept a source row only when it's on the cart and isn't a section header.
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};
