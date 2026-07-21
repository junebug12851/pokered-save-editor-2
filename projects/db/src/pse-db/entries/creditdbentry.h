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
#include <QQmlEngine>
#include <QString>
#include <QJsonValue>

#include "../db_autoport.h"

/**
 * @brief One credits entry -- and the canonical example of the "DB entry" pattern.
 *
 * @par The DB-entry convention (shared by every `*DBEntry` struct here)
 * - A QObject struct whose data fields are @b protected (write-once, set during
 *   load from JSON), exposed to QML and C++ through public getters wired as
 *   `Q_PROPERTY ... READ`. Always read via the getter (`getName()`), never the
 *   field.
 * - Protected constructors take the JSON (`QJsonValue&`) or a key; a static
 *   `process()` parses the JSON and appends entries to the owning DB's store.
 * - The owning DB is a `friend` (it constructs/populates entries).
 * - Richer entries also add a `deepLink()` that resolves cross-references after
 *   all DBs are loaded (e.g. a move's type, an evolution's target). Credits has
 *   no links, so it has none -- but the pattern is the same everywhere else.
 *
 * @see CreditsDB (its database), DB (the aggregate),
 *      [the db system map](../../../../notes/systems/db.md).
 */
struct DB_AUTOPORT CreditDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getSection READ getSection CONSTANT)   ///< Credits section heading.
  Q_PROPERTY(QString getName READ getName CONSTANT)         ///< Credited name.
  Q_PROPERTY(QString getUrl READ getUrl CONSTANT)           ///< Associated URL.
  Q_PROPERTY(QString getNote READ getNote CONSTANT)         ///< Free-text note.
  Q_PROPERTY(QString getLicense READ getLicense CONSTANT)   ///< License text/name.
  Q_PROPERTY(QString getMandated READ getMandated CONSTANT) ///< Mandated-attribution text.

public:
  QString getSection() const;  ///< @see getSection property.
  QString getName() const;     ///< @see getName property.
  QString getUrl() const;      ///< @see getUrl property.
  QString getNote() const;     ///< @see getNote property.
  QString getLicense() const;  ///< @see getLicense property.
  QString getMandated() const; ///< @see getMandated property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership (anti-GC).

protected:
  CreditDBEntry();                  ///< Empty entry (protected -- built by the DB).
  CreditDBEntry(QJsonValue& data);  ///< Build from a JSON value.
  CreditDBEntry(QString section);   ///< Build a section-header entry.

  static void process(QJsonObject& data); ///< Parse JSON and append entries to CreditsDB's store.
  void qmlRegister() const;                ///< Register this entry type with QML.

  QString section = "";  ///< Backing field (read via getSection()).
  QString name = "";     ///< Backing field (read via getName()).
  QString url = "";      ///< Backing field (read via getUrl()).
  QString note = "";     ///< Backing field (read via getNote()).
  QString license = "";  ///< Backing field (read via getLicense()).
  QString mandated = ""; ///< Backing field (read via getMandated()).

  friend class CreditsDB; ///< The owning DB constructs/populates these entries.
};
