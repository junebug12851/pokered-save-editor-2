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
 * @file settings.cpp
 * @brief Implementation of Settings -- UI theme/layout state. See settings.h.
 */

#include "./settings.h"
#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-db/tileset.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/area/area.h>
#include <pse-savefile/expanded/area/areamap.h>

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
  auto mapData = MapsDB::inst()->getIndAt(QString::number(mapInd));

  // Really shouldn't be any kind of error but stop here if there is one
  if(mapData == nullptr || mapData->getToTileset() == nullptr)
    return;

  // Load in the 2 settings
  previewTileset = mapData->getToTileset()->name;

  // All three states, not "is it Outdoor". TilesetType is INDOOR/CAVE/OUTDOOR = 0/1/2, the
  // same numbering as the cartridge's TILEANIM_NONE/WATER/WATER_FLOWER, so this is a
  // straight cast rather than a mapping. (It used to ask "== OUTDOOR" and hand back a bool,
  // which silently killed the water animation in every cave. See notes/reference/tiles.md.)
  previewTilesetType = static_cast<int>(mapData->getToTileset()->typeAsEnum());
}

void Settings::cyclePreviewTilesetType()
{
  previewTilesetType = (previewTilesetType + 1) % 3;
  previewTilesetTypeChanged();
}

QString Settings::previewTilesetTypeStr() const
{
  // What the tileset image provider's id wants (tilesetengine.cpp).
  switch(previewTilesetType) {
    case 1:  return "cave";
    case 2:  return "outdoor";
    default: return "indoor";
  }
}

QString Settings::previewTilesetTypeName() const
{
  switch(previewTilesetType) {
    case 1:  return tr("Cave");
    case 2:  return tr("Outdoor");
    default: return tr("Indoor");
  }
}

QString Settings::previewTilesetTypeDoes() const
{
  // Say what the setting DOES, not just what it's called -- the name is Twilight's friendly
  // rename of the cartridge's animation byte, and both halves are true and worth showing.
  switch(previewTilesetType) {
    case 1:  return tr("Water animates. Flowers don't.");
    case 2:  return tr("Water and flowers animate.");
    default: return tr("Nothing animates.");
  }
}
