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

#include "./settings.h"
#include <pse-db/maps.h>
#include <pse-db/tileset.h>
#include "../data/file/savefile.h"
#include "../data/file/expanded/savefileexpanded.h"
#include "../data/file/expanded/area/area.h"
#include "../data/file/expanded/area/areamap.h"

QString tilesetOrder[24] = {
  "Cavern",
  "Cemetery",
  "Club",
  "Dojo",
  "Facility",
  "Forest",
  "Forest Gate",
  "Gate",
  "Gym",
  "House",
  "Interior",
  "Lab",
  "Lobby",
  "Mansion",
  "Mart",
  "Museum",
  "Overworld",
  "Plateau",
  "Pokecenter",
  "Reds House 1",
  "Reds House 2",
  "Ship",
  "Ship Port",
  "Underground"
};

Settings::Settings(SaveFile* file)
  : file(file)
{
  // Only when all of the expanded data changes at once
  // the preview settings are kept seperate from the rest of the file so
  // we only want to update them when all the file data has been changed out
  connect(file, &SaveFile::dataExpandedChanged, this, &Settings::dataChanged);
}

void Settings::setColorScheme(QColor primary, QColor secondary)
{
  primaryColor = primary;
  primaryColorLight = primary.lighter(133);
  primaryColorDark = primary.darker(142);

  accentColor = secondary;
}

int Settings::getPreviewTilesetIndex()
{
  int ret = -1;

  for(int i = 0; i < 24; i++) {
    if(tilesetOrder[i] == previewTileset) {
      ret = i;
      break;
    }
  }

  return ret;
}

void Settings::dataChanged()
{
  // Get the current map
  // Even if no map data is laoded yet, the curMap will be 0 which is
  // Palette Town and works just fine as defaults
  auto mapInd = file->dataExpanded->area->map->curMap;

  // Get all the data for the current map
  auto mapData = MapsDB::ind.value(QString::number(mapInd), nullptr);

  // Really shouldn't be any kind of error but stop here if there is one
  if(mapData == nullptr || mapData->toTileset == nullptr)
    return;

  // Load in the 2 settings
  previewTileset = mapData->toTileset->name;
  previewOutdoor = mapData->toTileset->type == "Outdoor";
}
