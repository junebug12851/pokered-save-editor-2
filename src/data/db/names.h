/*
  * Copyright 2019 June Hanabi
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
#ifndef NAME_H
#define NAME_H

#include "../../common/types.h"
#include <QString>

// Something I made, random American names, I made it for the auto-nicknaming
// feature given this program is for the USA English Pokemon Red.

class Names
{
public:
  static void load();
  static QString randomName();

  static QVector<QString>* names;
};

#endif // NAME_H