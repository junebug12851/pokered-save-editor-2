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
#ifndef MAPCONNDATA_H
#define MAPCONNDATA_H

#include "../expandedinterface.h"
#include "../../../../common/types.h"
class SaveFile;
struct MapDBEntry;
struct MapDBEntryConnect;

class MapConnData : ExpandedInterface
{
public:
  MapConnData(SaveFile* saveFile = nullptr, var16 offset = 0);
  virtual ~MapConnData();

  void load(SaveFile* saveFile = nullptr, var16 offset = 0);
  void save(SaveFile* saveFile, var16 offset);
  void reset();
  void loadFromData(MapDBEntryConnect* connect);

  var8 mapPtr;
  var16 stripSrc;
  var16 stripDst;
  var8 stripWidth;
  var8 width;
  var8 yAlign;
  var8 xAlign;
  var16 viewPtr;

  MapDBEntry* toMap();

private:
  // To surpress warnings with using the ExpandedInterface contract
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void randomize();
};

#endif // MAPCONNDATA_H
