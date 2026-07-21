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
#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QVector>

/**
 * @brief Exposes the credits database to QML as a list of sections, each carrying
 *        its own entries -- so the About screen can render one card per category.
 *
 * @par Section-grouped, not flat
 * The underlying CreditsDB store is flat (a section-header sentinel entry followed
 * by that section's entries). A flat list model forces the view to interleave
 * headers and rows by hand; for grouped "section cards" the natural shape is one
 * model row per section. So this model regroups the flat store on first use:
 * - **section** -- the category name (the card heading);
 * - **entries** -- a `QVariantList` of `{name,url,note,license,mandated}` maps,
 *   iterated by a `Repeater` inside the card.
 *
 * @par The mvc-model convention (shared by every model here)
 * Each is a `QAbstractListModel` that adapts a C++ collection (a DB store or a save
 * object) for a QML `ListView`/`Repeater`, overriding rowCount()/data()/roleNames().
 *
 * @see CreditsDB (the source), Bridge (exposes this as `brg.creditsModel`).
 */
class CreditsModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// The roles QML can bind to (mapped to names in roleNames()).
  enum CreditRoles {
    SectionRole = Qt::UserRole + 1, ///< Category name (the card heading).
    EntriesRole                     ///< QVariantList of this section's entry maps.
  };

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Number of sections.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Value for a section + role.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name map.

private:
  /// One credits category: its heading and the entry maps under it.
  struct Section {
    QString name;          ///< Category heading.
    QVariantList entries;  ///< {name,url,note,license,mandated} maps.
  };

  /// Regroup the flat CreditsDB store into @ref m_sections (once, lazily).
  void ensureBuilt() const;

  mutable QVector<Section> m_sections; ///< Cached grouped view of the store.
  mutable bool m_built = false;        ///< Whether @ref m_sections is populated.
};
