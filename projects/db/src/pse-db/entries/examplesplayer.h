/*
  * Copyright 2020 June Hanabi
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
#ifndef EXAMPLESPLAYER_H
#define EXAMPLESPLAYER_H

#include <QObject>
#include <QString>

#include "./abstractrandomstring.h"
#include "../db_autoport.h"

class QQmlEngine;

class DB_AUTOPORT ExamplesPlayer : public AbstractRandomString
{
  Q_OBJECT

public:
  // Get Instance
  static ExamplesPlayer* inst();

protected slots:
  ExamplesPlayer();
  virtual void qmlRegister() const;
};

#endif // EXAMPLESPLAYER_H
