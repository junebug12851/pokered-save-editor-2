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

#include "./halloffame.h"
#include "../savefile.h"
#include "../savefiletoolset.h"
#include "../savefileiterator.h"
#include "./fragments/hofrecord.h"
#include "../../../random.h"

HallOfFame::HallOfFame(SaveFile* saveFile)
{
  load(saveFile);
}

HallOfFame::~HallOfFame()
{
  reset();
}

void HallOfFame::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  var8 hofRecordCount = toolset->getByte(0x284E);
  for (var8 i = 0; i < hofRecordCount && i < recordsMax; i++) {
    records.append(new HoFRecord(saveFile, i));
  }
}

void HallOfFame::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x284E, records.size());
  for (var8 i = 0; i < records.size() && i < recordsMax; i++) {
    records.at(i)->save(saveFile, i);
  }
}

void HallOfFame::reset()
{
  for(auto record : records)
    delete record;

  records.clear();
}

void HallOfFame::randomize()
{
  reset();

  // Create up to 5 random records
  var8 rndCount = Random::rangeInclusive(0, 5);

  for (var8 i = 0; i < rndCount && i < recordsMax; i++) {
    auto tmp = new HoFRecord;
    records.append(tmp);
    tmp->randomize();
  }
}
