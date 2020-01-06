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

#include <QRandomGenerator>

AreaAudio::AreaAudio(SaveFile* saveFile)
{
  load(saveFile);
}

AreaAudio::~AreaAudio() {}

void AreaAudio::load(SaveFile* saveFile)
{
  if(saveFile == nullptr)
    return reset();
}

void AreaAudio::save(SaveFile* saveFile)
{

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
  auto rnd = QRandomGenerator::global();

  noAudioFadeout = rnd->bounded(0, 5) > 3;
  preventMusicChange = rnd->bounded(0, 5) > 3;
}
