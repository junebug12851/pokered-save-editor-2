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
#include "player.h"
#include "../../savefile.h"
#include "./playerbasics.h"

Player::Player(SaveFile* saveFile)
{
  basics = new PlayerBasics;

  load(saveFile);
}

Player::~Player()
{
  delete basics;
}

void Player::load(SaveFile* saveFile)
{
  basics->load(saveFile);
}

void Player::save(SaveFile* saveFile)
{
  basics->save(saveFile);
}

void Player::reset()
{
  basics->reset();
}

void Player::randomize()
{
  basics->randomize();
}
