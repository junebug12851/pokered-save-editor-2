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
#include "mapdbentrysign.h"

MapDBEntrySign::MapDBEntrySign() {}
MapDBEntrySign::MapDBEntrySign(QJsonValue& data, MapDBEntry* parent) :
  parent(parent)
{
  x = data["x"].toDouble();
  y = data["y"].toDouble();
  textID = data["text"].toDouble();
}
