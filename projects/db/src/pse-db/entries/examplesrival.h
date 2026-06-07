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

#include "./abstractrandomstring.h"
#include "../db_autoport.h"

class QQmlEngine;

/**
 * @brief Example rival-value source (an AbstractRandomString of presets).
 *
 * All behaviour inherited; provides the singleton + QML registration. Reached via
 * `db.examples.rival`.
 *
 * @see AbstractRandomString, Examples.
 */
class DB_AUTOPORT ExamplesRival : public AbstractRandomString
{
  Q_OBJECT

public:
  // Get Instance
  static ExamplesRival* inst(); ///< The process-wide ExamplesRival singleton.

protected:
  ExamplesRival(); ///< Private -- use inst(); loads the example-rival asset.

protected slots:
  virtual void qmlRegister() const; ///< Register ExamplesRival with QML.
};
