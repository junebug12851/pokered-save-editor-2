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

#include <QRandomGenerator>

#include "./random.h"

var32 Random::rangeInclusive(var32 start, var32 end)
{
  return rnd->bounded(start, end+1);
}

var32 Random::rangeExclusive(var32 start, var32 end)
{
  return rnd->bounded(start, end);
}

bool Random::chanceFailure(var8 percent)
{
  return rnd->bounded(0, 100) > percent;
}

bool Random::chanceSuccess(var8 percent)
{
  return rnd->bounded(0, 100) < percent;
}

QRandomGenerator* Random::rnd = QRandomGenerator::global();
