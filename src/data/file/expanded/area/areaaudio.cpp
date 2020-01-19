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
#include "./areaaudio.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/music.h"
#include "../../../../random.h"

AreaAudio::AreaAudio(SaveFile* saveFile)
{
  load(saveFile);
}

AreaAudio::~AreaAudio() {}

void AreaAudio::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();

  auto toolset = saveFile->toolset;

  musicID = toolset->getByte(0x2607);
  musicBank = toolset->getByte(0x2608);
  noAudioFadeout = toolset->getBit(0x29D8, 1, 1);
  preventMusicChange = toolset->getBit(0x29DF, 1, 1);
}

void AreaAudio::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setByte(0x2607, musicID);
  toolset->setByte(0x2608, musicBank);
  toolset->setBit(0x29D8, 1, 1, noAudioFadeout);
  toolset->setBit(0x29DF, 1, 1, preventMusicChange);
}

void AreaAudio::reset()
{
  musicID = 0;
  musicBank = 0;
  noAudioFadeout = false;
  preventMusicChange = false;
}

void AreaAudio::randomize()
{
  // Select a random song
  auto musicEntry = MusicDB::store.at(Random::rangeExclusive(0, MusicDB::store.size()));

  // Load it into the map
  musicID = musicEntry->id;
  musicBank = musicEntry->bank;

  noAudioFadeout = false;
  preventMusicChange = false;
}
