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

#include "./random.h"
#include "utility.h"
#include <QQmlEngine>

Random* Random::inst()
{
  static Random* _inst = new Random;
  return _inst;
}

float Random::range(const float end) const
{
  return rnd->bounded(end);
}

int Random::rangeInclusive(const int start, const int end) const
{
  if(start == end || start > end)
    return start;

  return rnd->bounded(start, end+1);
}

int Random::rangeExclusive(const int start, const int end) const
{
  if(start == end || start > end)
    return start;

  return rnd->bounded(start, end);
}

bool Random::chanceFailure(const int percent) const
{
  return rangeInclusive(0, 100) >= percent;
}

bool Random::chanceFailure(const float percent) const
{
  return range(1.0f) >= percent;
}

bool Random::chanceSuccess(const int percent) const
{
  return rangeInclusive(0, 100) <= percent;
}

bool Random::chanceSuccess(const float percent) const
{
  return range(1.0f) <= percent;
}

bool Random::flipCoin() const
{
  return chanceSuccess(50);
}

bool Random::flipCoinF() const
{
  return chanceSuccess(0.5f);
}

void Random::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void Random::qmlRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<Random>("PSE.Common.Random", 1, 0, "Random", "Can't instantiate in QML");
  registered = true;
}

Random::Random()
{
  qmlRegister();
}
