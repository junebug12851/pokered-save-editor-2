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
#include <QJsonDocument>
#include <QByteArray>

#include "../db_autoport.h"

class QQmlEngine;

// This helps with getting game data from the JSON files

/**
 * @brief The JSON asset loader -- the raw data source behind every database.
 *
 * A singleton that reads the bundled `/assets/data/.json` files from disk. Each
 * DB's load() calls into this to get its document. Exposed to QML as `db.json`.
 *
 * @see DB (the aggregate), and every `*DB::load()`.
 */
class DB_AUTOPORT GameData : public QObject
{
  Q_OBJECT

public:
  // Get Instance
  static GameData* inst(); ///< The process-wide GameData singleton.

  // Retrieves JSON document from disk
  // Passed by value because of Qt's COW principle ensures no speed loss
  // Give the name of the file in /assets/data without the .json file extension
  const QByteArray jsonRaw(const QString filename) const;   ///< Raw bytes of @p filename (no .json extension).
  const QJsonDocument json(const QString filename) const;   ///< Parsed document for @p filename.
  Q_INVOKABLE const QString jsonStr(const QString filename) const; ///< @p filename as a string (for QML).

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private:
  GameData(); ///< Private -- use inst().

  void qmlRegister() const; ///< Register with the QML type system.
};
