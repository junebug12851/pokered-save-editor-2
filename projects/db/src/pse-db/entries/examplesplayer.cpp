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

#include <QVector>
#include <QJsonArray>
#include <QtMath>
#include <QQmlEngine>

#include "./examplesplayer.h"
#include "../util/gamedata.h"
#include <pse-common/random.h>
#include <pse-common/utility.h>

void ExamplesPlayer::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<ExamplesPlayer>("PSE.DB.ExamplesPlayer", 1, 0, "ExamplesPlayer", "Can't instantiate in QML");
  once = true;
}

ExamplesPlayer::ExamplesPlayer()
  : AbstractExample("playerExamples")
{
  qmlRegister();
}

ExamplesPlayer* ExamplesPlayer::inst()
{
  static ExamplesPlayer* _inst = new ExamplesPlayer;
  return _inst;
}
