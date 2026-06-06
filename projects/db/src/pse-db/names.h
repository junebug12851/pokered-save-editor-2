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

#include "./db_autoport.h"
#include "./entries/namesplayer.h"
#include "./entries/namespokemon.h"

class QQmlEngine;

class DB_AUTOPORT Names : public QObject
{
  Q_OBJECT
  Q_PROPERTY(NamesPlayer*  player  READ player  CONSTANT)
  Q_PROPERTY(NamesPokemon* pokemon READ pokemon CONSTANT)

public:
  static Names* inst();

  NamesPlayer*  player()  const;
  NamesPokemon* pokemon() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  Names();
};

