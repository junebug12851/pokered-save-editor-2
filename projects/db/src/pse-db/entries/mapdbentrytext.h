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
#include <QString>
#include <QJsonValue>
#include "../db_autoport.h"

class MapDBEntry;
class QQmlEngine;

/**
 * @brief One entry of a map's text-pointer table -- the words behind a text id.
 *
 * A sign stores a **text id**, a 1-based index into the map's `def_text_pointers` table.
 * This is one row of that table, imported from `pret/pokered` by
 * `scripts/import_sign_text.py`: the id, the actual @ref getText string, whether it is a
 * @ref getScripted `text_asm` entry with no single literal, and which @ref getCategory it
 * falls in (sign / person / other) so the sign picker can group it.
 *
 * @see MapDBEntry (parent), MapDBEntrySign (the save-side sign that points at one of these),
 *      notes/reference/signs.md.
 */
struct DB_AUTOPORT MapDBEntryText : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getId READ getId CONSTANT)                  ///< 1-based text id (index into the table).
  Q_PROPERTY(QString getText READ getText CONSTANT)          ///< The words (empty when scripted).
  Q_PROPERTY(bool getScripted READ getScripted CONSTANT)     ///< True = a `text_asm` script, no single literal.
  Q_PROPERTY(QString getCategory READ getCategory CONSTANT)  ///< "sign" | "person" | "other".
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT)  ///< Owning map.

public:
  int getId() const;               ///< @see getId property.
  const QString getText() const;   ///< @see getText property.
  bool getScripted() const;        ///< @see getScripted property.
  const QString getCategory() const; ///< @see getCategory property.
  MapDBEntry* getParent() const;   ///< @see getParent property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntryText(); ///< Empty entry.
  MapDBEntryText(const QJsonValue& data,
                 MapDBEntry* const parent); ///< Build from JSON under @p parent.
  void qmlRegister() const; ///< Register with QML.

  int id = 0;             ///< 1-based text id.
  QString text = "";      ///< The words (empty when scripted).
  bool scripted = false;  ///< A `text_asm` script with no literal.
  QString category = "other"; ///< sign / person / other.

  MapDBEntry* parent = nullptr; ///< Owning map.

  friend class MapDBEntry;
};
