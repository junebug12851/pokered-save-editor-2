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

#include "./examplesrival.h"
#include "../util/gamedata.h"
#include <pse-common/random.h>
#include <pse-common/utility.h>

void ExamplesRival::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<ExamplesRival>("PSE.DB.ExamplesRival", 1, 0, "ExamplesRival", "Can't instantiate in QML");
  once = true;
}

ExamplesRival::ExamplesRival()
  : AbstractExample("rivalExamples")
{
  qmlRegister();
}

ExamplesRival* ExamplesRival::inst()
{
  static ExamplesRival* _inst = new ExamplesRival;
  return _inst;
}
