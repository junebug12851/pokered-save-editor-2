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
