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
#include <QVector>
#include <QQmlListProperty>
#include <pse-common/types.h>
#include "../savefile_autoport.h"

class SaveFile;
class HoFRecord;

constexpr var8 recordsMax = 50; ///< Maximum Hall of Fame records the save keeps.

/**
 * @brief The Hall of Fame: a rolling list of up to 50 winning-team records.
 *
 * Holds the @ref records list of HoFRecord entries (each a team that beat the
 * Elite Four). Provides QML-callable add/remove/swap/access. Standard
 * expanded-node convention (see SaveFileExpanded).
 *
 * @see SaveFileExpanded, HoFRecord, HoFPokemon.
 */
class SAVEFILE_AUTOPORT HallOfFame : public QObject
{
  Q_OBJECT

public:
  HallOfFame(SaveFile* saveFile = nullptr);
  virtual ~HallOfFame();

  void load(SaveFile* saveFile = nullptr); ///< Expand the Hall of Fame records from the save.
  void save(SaveFile* saveFile);           ///< Flatten the Hall of Fame records to the save.

  // Since Qt has tied my hands in so many ways on fixing the issue of no arrays
  // but primitive arrays being able to be sent to QML, I'm left with no other
  // options outside of a custom model to flood classes with a series of methods
  // for each and every array of any kind that's not primitive
  Q_INVOKABLE int recordCount();             ///< Number of records.
  Q_INVOKABLE int recordMax();               ///< Capacity (recordsMax).
  Q_INVOKABLE HoFRecord* recordAt(int ind);  ///< Record @p ind (GC-protected return).
  Q_INVOKABLE void recordSwap(int from, int to); ///< Reorder records.
  Q_INVOKABLE void recordRemove(int ind);    ///< Remove record @p ind.
  Q_INVOKABLE void recordNew();              ///< Add a fresh record.

signals:
  void recordsChanged();

public slots:
  void reset();     ///< Clear all records.
  void randomize(); ///< Fill with random records.

public:
  // All Hall of Fame Records
  QVector<HoFRecord*> records; ///< Every Hall of Fame record.
};
