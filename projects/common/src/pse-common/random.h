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
#ifndef RANDOM_H
#define RANDOM_H

#include "./common_autoport.h"

class QRandomGenerator;

class COMMON_AUTOPORT Random
{
public:
  // [from,to]
  static int rangeInclusive(int start, int end);

  // [from,to)
  static int rangeExclusive(int start, int end);

  // Chance of failure, 0 - 100
  static bool chanceFailure(int percent);

  // Chance of success, 0 - 100
  static bool chanceSuccess(int percent);

  // Chance of success, 50%
  static bool flipCoin();

  static QRandomGenerator* rnd;
};

#endif // RANDOM_H
