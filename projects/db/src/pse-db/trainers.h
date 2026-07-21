/*
  * Copyright 2019 Fairy Fox
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
#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MapDBEntrySpriteTrainer;
class QQmlEngine;

/**
 * @brief One trainer-class definition (its name/index and flags).
 *
 * Plain-struct DB entry. @ref tpMapSpriteTrainers is a back-reference of every
 * on-map trainer sprite of this class (filled when MapsDB deep-links). See db.md.
 *
 * @see TrainersDB, MapDBEntrySpriteTrainer.
 */
struct DB_AUTOPORT TrainerDBEntry {
  TrainerDBEntry();                ///< Empty entry.
  TrainerDBEntry(QJsonValue& data); ///< Build from a JSON value.

  QString name;        ///< Trainer-class name (key).
  var8 ind    = 0;     ///< Trainer-class index.
  bool unused = false; ///< Whether this slot is unused.
  bool opp    = false; ///< Whether it's an opponent class.

  QVector<MapDBEntrySpriteTrainer*> tpMapSpriteTrainers; ///< On-map trainers of this class (back-ref).
};

/**
 * @brief The trainers database -- every trainer class, keyed by name.
 *
 * Standard DB-singleton with a name index (see CreditsDB / db.md).
 *
 * @see TrainerDBEntry, DB.
 */
class DB_AUTOPORT TrainersDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT) ///< Number of trainer classes.

public:
  static TrainersDB* inst(); ///< The process-wide TrainersDB singleton.

  [[nodiscard]] const QVector<TrainerDBEntry*> getStore() const;       ///< All trainer classes.
  [[nodiscard]] const QHash<QString, TrainerDBEntry*> getInd() const;  ///< Name->entry index.
  [[nodiscard]] int getStoreSize() const;                            ///< Trainer-class count.

  Q_INVOKABLE TrainerDBEntry* getStoreAt(int idx) const;            ///< Trainer by store index (for QML).
  Q_INVOKABLE TrainerDBEntry* getIndAt(const QString& key) const;   ///< Trainer by name key (for QML).

public slots:
  void load();   ///< Load trainers from JSON.
  void index();  ///< Build the name->entry index.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  TrainersDB(); ///< Private -- use inst().

  QVector<TrainerDBEntry*> store;       ///< The loaded trainer classes.
  QHash<QString, TrainerDBEntry*> ind;  ///< Name->entry lookup.
};
