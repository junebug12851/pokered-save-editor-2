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

#include <QQmlEngine>
#include <pse-common/utility.h>
#include "examples.h"

#include "./entries/examplesplayer.h"
#include "./entries/examplesrival.h"
#include "./entries/examplespokemon.h"

Examples* Examples::inst()
{
  static Examples* _inst = new Examples;
  return _inst;
}

const ExamplesPlayer* Examples::player() const
{
  return ExamplesPlayer::inst();
}

const ExamplesRival* Examples::rival() const
{
  return ExamplesRival::inst();
}

const ExamplesPokemon* Examples::pokemon() const
{
  return ExamplesPokemon::inst();
}

void Examples::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);

  ExamplesPlayer::inst()->qmlProtect(engine);
  ExamplesRival::inst()->qmlProtect(engine);
  ExamplesPokemon::inst()->qmlProtect(engine);
}

void Examples::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<Examples>("PSE.DB.Examples", 1, 0, "Examples", "Can't instantiate in QML");
  once = true;
}

Examples::Examples()
{
  qmlRegister();
}
