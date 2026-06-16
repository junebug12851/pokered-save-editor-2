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
 * @brief The left-list VIEW of @ref ItemMarketModel -- the Buy or Sell slice.
 *
 * The unified market list holds BOTH the sell rows (your items) and the buy rows
 * (the store) at once -- one single-currency cart spanning both. The Buy/Sell strip
 * only chooses which slice the left list shows; it must NOT rebuild or clear the
 * cart. This proxy does that slicing: it accepts source rows whose `viewTag` matches
 * the model's current `isBuyMode` (ViewBuy vs ViewSell), and re-filters live when the
 * strip toggles. In Exchange mode the source is the separate exchange list, shown
 * wholesale. The receipt reads the cart across both slices via @ref ItemMarketCartModel.
 *
 * Exposed as `brg.marketViewModel`; the left `ListView` binds to it. Roles pass
 * through from the source unchanged.
 *
 * @see ItemMarketModel, ItemMarketCartModel.
 */
class ItemMarketViewModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  explicit ItemMarketViewModel(ItemMarketModel* source, QObject* parent = nullptr);

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
  ItemMarketModel* m_market = nullptr; ///< Source, kept typed for the live mode flags.
};
