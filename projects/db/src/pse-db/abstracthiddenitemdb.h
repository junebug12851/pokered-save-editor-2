/*
  * Copyright 2019 Twilight
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
#include "./db_autoport.h"

class HiddenItemDBEntry;
class QQmlEngine;

/**
 * @brief Shared base for the two hidden-pickup databases (items and coins).
 *
 * Hidden items and hidden coins are the same shape -- a store of HiddenItemDBEntry
 * loaded from a JSON file -- so the common machinery (store, load, deepLink, QML
 * accessors) lives here once. Subclasses (HiddenItemsDB, HiddenCoinsDB) just supply
 * their @ref loadFile (via the protected constructor), an inst(), and the concrete
 * qmlRegister(). This is the one DB family using inheritance rather than the
 * copy-the-pattern approach. See db.md.
 *
 * @see HiddenItemsDB, HiddenCoinsDB, HiddenItemDBEntry.
 */
class DB_AUTOPORT AbstractHiddenItemDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of hidden pickups.

public:
  // Get Properties, includes QML array helpers
  const QVector<HiddenItemDBEntry*> getStore() const; ///< All hidden-pickup entries.
  int getStoreSize() const;                           ///< Entry count.

   Q_INVOKABLE HiddenItemDBEntry* getStoreAt(const int ind) const; ///< Entry by store index (for QML).

public slots:
  void load();     ///< Load entries from @ref loadFile.
  void deepLink(); ///< Resolve each entry's cross-DB links.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected slots:
  virtual void qmlRegister() const = 0; ///< Subclass registers its concrete type with QML.

protected:
  /// @param loadFile the JSON asset the concrete subclass loads from.
  /// @param isCoin   true for HiddenCoinsDB, false for HiddenItemsDB. Stamped onto every entry
  ///                 so a pickup knows which save array its index refers to (the two are
  ///                 separate bitfields with independent numbering). Passed explicitly rather
  ///                 than sniffed from @p loadFile -- a filename is not a type.
  AbstractHiddenItemDB(const QString loadFile, bool isCoin);

  QVector<HiddenItemDBEntry*> store; ///< The loaded entries.
  const QString loadFile;            ///< JSON asset path (set by the subclass).
  const bool isCoin;                 ///< Is this the coins DB? (set by the subclass).

  /// ⚠️ These MUST stay per-instance members, never `static` locals inside load()/deepLink().
  /// This base is shared by two singletons; a static local here is one flag for BOTH, so the
  /// second DB constructed would skip its own load entirely. That was a real, long-lived bug --
  /// it left HiddenItemsDB empty. See abstracthiddenitemdb.cpp.
  bool loaded = false;     ///< Has this DB loaded its own file yet?
  bool deepLinked = false; ///< Has this DB resolved its own entries' links yet?
};
