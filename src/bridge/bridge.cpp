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

#include "./bridge.h"
#include "../data/file/savefile.h"
#include "../data/file/expanded/savefileexpanded.h"
#include "../data/file/expanded/player/player.h"
#include "../data/file/expanded/player/playerpokedex.h"

Bridge::Bridge(FileManagement* file)
  : file(file),
    recentFilesModel(new RecentFilesModel(file)),
    pokedexModel(new PokedexModel(file->data->dataExpanded->player->pokedex))
{}
