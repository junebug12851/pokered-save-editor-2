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

#include "../db_autoport.h"

class QQmlEngine;

/**
 * @brief Shared base for a simple "list of strings, pick a random one" source.
 *
 * The base for NamesPlayer and NamesPokemon: a @ref store of strings loaded from a
 * @ref fileName, with a @ref randomExample() helper. Subclasses just supply the
 * file (via the protected ctor), an inst(), and qmlRegister().
 *
 * @note Unusually for this layer, the store isn't strictly read-only -- hence the
 *       @ref listChanged signal (see the inline note).
 * @see NamesPlayer, NamesPokemon, Names (the aggregate).
 */
class DB_AUTOPORT AbstractRandomString : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize STORED false NOTIFY listChanged) ///< Number of strings.

signals:
  // Sort of an exception, the DB is almost entirely read-only but this is an
  // exception of some moving parts in the DB
  void listChanged(); ///< Emitted when the string list changes (rare; see note).

public:
  const QVector<QString> getStore() const;             ///< All strings.
  int getStoreSize() const;                            ///< String count.
  Q_INVOKABLE const QString getStoreAt(const int ind) const; ///< String at @p ind (for QML).

  Q_INVOKABLE QString randomExample(); ///< A random string from the list.

public slots:
  // QML accessible methods
  void load();                                         ///< Load the strings from @ref fileName.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected slots:
  virtual void qmlRegister() const = 0; ///< Subclass registers its concrete type with QML.

protected:
  /// @param fileName the JSON/text asset the subclass loads from.
  AbstractRandomString(const QString fileName);
  QVector<QString> store; ///< The loaded strings.
  const QString fileName; ///< Asset path (set by the subclass).
};
