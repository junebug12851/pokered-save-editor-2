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
#include "./abstracthiddenitemdb.h"

/**
 * @brief The hidden-coins database -- AbstractHiddenItemDB loaded from the coins file.
 *
 * All behaviour is inherited; this subclass just provides the singleton and the
 * concrete QML registration. See AbstractHiddenItemDB.
 *
 * @see AbstractHiddenItemDB, WorldHidden (the save-side hidden-coin flags).
 */
class HiddenCoinsDB : public AbstractHiddenItemDB
{
  Q_OBJECT

public:
  static HiddenCoinsDB* inst(); ///< The process-wide HiddenCoinsDB singleton.

protected slots:
  virtual void qmlRegister() const; ///< Register HiddenCoinsDB with QML.

protected:
  HiddenCoinsDB(); ///< Private -- use inst(); passes the coins JSON to the base.
};
