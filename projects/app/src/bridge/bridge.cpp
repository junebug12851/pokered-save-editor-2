/*
  * Copyright 2020 Twilight
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

/**
 * @file bridge.cpp
 * @brief Implementation of Bridge -- constructs the models and wires up the
 *        `brg` aggregate. See bridge.h for the documented API.
 */

#include "./bridge.h"
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areageneral.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areatileset.h>
#include <pse-savefile/expanded/world/world.h>
#include <pse-savefile/expanded/world/worldgeneral.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokedex.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>

Bridge::Bridge(FileManagement* file)
  : file(file),
    recentFilesModel(new RecentFilesModel(file)),
    pokedexModel(new PokedexModel(file->data->dataExpanded->player->pokedex, router)),
    bagItemsModel(new ItemStorageModel(file->data->dataExpanded->player->items, router)),
    pcItemsModel(new ItemStorageModel(file->data->dataExpanded->storage->items, router)),
    itemOverviewModel(new ItemOverviewModel(file->data->dataExpanded->player->items, file->data->dataExpanded->storage->items)),
    itemExchangeModel(new ItemExchangeModel(file->data->dataExpanded->player->items, file->data->dataExpanded->storage->items, file->data->dataExpanded->player->basics)),
    pokemonOverviewModel(new PokemonOverviewModel(file->data->dataExpanded->player->pokemon, file->data->dataExpanded->storage, file->data->dataExpanded->player->basics)),
    marketModel(new ItemMarketModel(
                  file->data->dataExpanded->player->items,
                  file->data->dataExpanded->storage->items,
                  file->data->dataExpanded->player->basics,
                  router,
                  file->data->dataExpanded->player->pokemon,
                  file->data->dataExpanded->storage,
                  file->data
                  )),
    marketCartModel(new ItemMarketCartModel(marketModel)),
    marketViewModel(new ItemMarketViewModel(marketModel)),
    pokemonStorageModel1(new PokemonStorageModel(router, file->data->dataExpanded->storage, file->data->dataExpanded->player->pokemon)),
    pokemonStorageModel2(new PokemonStorageModel(router, file->data->dataExpanded->storage, file->data->dataExpanded->player->pokemon)),
    map(new MapModel(file->data->dataExpanded->area->map,
                     file->data->dataExpanded->area->player,
                     file->data->dataExpanded->area->tileset,
                     file->data->dataExpanded->area->general,
                     file->data->dataExpanded->area->preloadedSprites,
                     file->data->dataExpanded->area->sprites,
                     file->data->dataExpanded->area->warps,
                     // ⭐ `wLastMap` + `wLastBlackoutMap` -- the two most consequential warp bytes
                     // in the save -- live in WorldGeneral, not AreaWarps, which is exactly why
                     // nobody editing warps ever saw them. See notes/reference/warps.md §4.
                     file->data->dataExpanded->world->general,
                     file->data->dataExpanded->area->signs,
                     // The wild-encounter cooldown flag lives in the encounter block.
                     file->data->dataExpanded->area->pokemon,
                     // The map-states surface: the whole World (scripts/events/missables),
                     // the whole Area (seamless map-change construction) and the trainer's
                     // basics (the badge bits). See notes/plans/map-states.md.
                     file->data->dataExpanded->world,
                     file->data->dataExpanded->area,
                     file->data->dataExpanded->player->basics)),
    mapLayers(new MapLayersModel(map)),
    mapClock(new MapClock(map)),
    mapSim(new MapSim(map))
{
  // Link the two
  pokemonStorageModel1->otherModel = pokemonStorageModel2;
  pokemonStorageModel2->otherModel = pokemonStorageModel1;

  // Link the two item models too (for drag-to-transfer between the bag and the
  // PC item box).
  bagItemsModel->otherModel = pcItemsModel;
  pcItemsModel->otherModel = bagItemsModel;

  // Switch the second half to box 0, the two halves don't need to show the same
  // box
  pokemonStorageModel2->switchBox(0);

  // Setup paired select boxes after storage models are created and init
  pokemonBoxSelectModel1 = new PokemonBoxSelectModel(pokemonStorageModel1);
  pokemonBoxSelectModel2 = new PokemonBoxSelectModel(pokemonStorageModel2);
}
