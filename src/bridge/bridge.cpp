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
#include "../data/file/expanded/area/area.h"
#include "../data/file/expanded/area/areamap.h"
#include "../data/file/expanded/player/player.h"
#include "../data/file/expanded/player/playerpokedex.h"
#include "../data/file/expanded/storage.h"
#include "../data/file/expanded/fragments/itemstoragebox.h"

Bridge::Bridge(FileManagement* file)
  : file(file),
    recentFilesModel(new RecentFilesModel(file)),
    pokedexModel(new PokedexModel(file->data->dataExpanded->player->pokedex, router)),
    bagItemsModel(new ItemStorageModel(file->data->dataExpanded->player->items, router)),
    pcItemsModel(new ItemStorageModel(file->data->dataExpanded->storage->items, router)),
    marketModel(new ItemMarketModel(
                  file->data->dataExpanded->player->items,
                  file->data->dataExpanded->storage->items,
                  file->data->dataExpanded->player->basics,
                  router,
                  file->data->dataExpanded->player->pokemon,
                  file->data->dataExpanded->storage,
                  file->data
                  )),
    pokemonStorageModel1(new PokemonStorageModel(router, file->data->dataExpanded->storage, file->data->dataExpanded->player->pokemon)),
    pokemonStorageModel2(new PokemonStorageModel(router, file->data->dataExpanded->storage, file->data->dataExpanded->player->pokemon)),
    mapSelectModel(new MapSelectModel(file->data->dataExpanded->area->map))
{
  // Link the two
  pokemonStorageModel1->otherModel = pokemonStorageModel2;
  pokemonStorageModel2->otherModel = pokemonStorageModel1;

  // Switch the second half to box 0, the two halves don't need to show the same
  // box
  pokemonStorageModel2->switchBox(0);

  // Setup paired select boxes after storage models are created and init
  pokemonBoxSelectModel1 = new PokemonBoxSelectModel(pokemonStorageModel1);
  pokemonBoxSelectModel2 = new PokemonBoxSelectModel(pokemonStorageModel2);
}
