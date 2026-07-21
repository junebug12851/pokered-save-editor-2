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
#include <QString>

#include "./abstractrandomstring.h"
#include "../db_autoport.h"

class QQmlEngine;

/**
 * @brief Random Pokemon-nickname source (an AbstractRandomString of names).
 *
 * All behaviour is inherited; this just provides the singleton + QML registration.
 * Backs `db.names.pokemon` and the nickname randomizer.
 *
 * @see AbstractRandomString, Names.
 */
class DB_AUTOPORT NamesPokemon : public AbstractRandomString
{
  Q_OBJECT

public:
  // Get Instance
  static NamesPokemon* inst(); ///< The process-wide NamesPokemon singleton.

protected:
  NamesPokemon(); ///< Private -- use inst(); loads the pokemon-names asset.

protected slots:
  virtual void qmlRegister() const; ///< Register NamesPokemon with QML.
};
