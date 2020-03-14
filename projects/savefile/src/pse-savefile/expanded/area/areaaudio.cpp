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
#include <pse-db/music.h>
#include <pse-db/maps.h>
#include <pse-common/random.h>

AreaAudio::AreaAudio(SaveFile* saveFile)
{
  load(saveFile);
}

AreaAudio::~AreaAudio() {}

void AreaAudio::load(SaveFile* saveFile)
{
  reset();
  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  musicID = toolset->getByte(0x2607);
  musicIDChanged();

  musicBank = toolset->getByte(0x2608);
  musicBankChanged();

  noAudioFadeout = toolset->getBit(0x29D8, 1, 1);
  noAudioFadeoutChanged();

  preventMusicChange = toolset->getBit(0x29DF, 1, 1);
  preventMusicChangeChanged();
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
  musicIDChanged();

  musicBank = 0;
  musicBankChanged();

  noAudioFadeout = false;
  noAudioFadeoutChanged();

  preventMusicChange = false;
  preventMusicChangeChanged();
}

void AreaAudio::randomize()
{
  reset();

  // Select a random song
  auto musicEntry = MusicDB::store.at(Random::rangeExclusive(0, MusicDB::store.size()));

  // Load it into the map
  musicID = musicEntry->id;
  musicIDChanged();

  musicBank = musicEntry->bank;
  musicBankChanged();
}

void AreaAudio::setTo(MapDBEntry* map)
{
  reset();

  // Select a random song
  MusicDBEntry* musicEntry = map->toMusic;

  // Load it into the map
  musicID = (musicEntry == nullptr)
      ? 0
      : musicEntry->id;
  musicIDChanged();

  musicBank = musicID = (musicEntry == nullptr)
      ? 0
      : musicEntry->bank;
  musicBankChanged();
}
