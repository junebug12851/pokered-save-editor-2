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
#ifndef INDIVIDUALMAP_H
#define INDIVIDUALMAP_H

#include <QObject>

/*
 * So this class is a bit wierd, it's technically an MVC, but not Qt's MVC and
 * it enables the view to work with code all over the place but the view is in
 * charge and this is the helper.
 *
 * The expanded data is setup to be data oriented, not map oriented. So you have
 * all the events for all the maps and all the hidden items for all the maps,
 * etc...
 *
 * This class exists if you want to work with all the data but just for one map.
 * It's here to allow the view to ask it questions and make requests. It handles
 * the bulk of the work for the view but the view is in charge.
*/

class IndividualMap : public QObject
{
  Q_OBJECT

public:
  IndividualMap();

  /*
   * Name
   * Ind
   * Glitch
   * Special
   * Warps
   * Signs
   * Sprites
   * Connecting
   * Pokemon
   * Sprite Set
   * Border
   * Pointers
   * Size
   * Music
   * Tileset
   * Incomplete
   * Events
   * A Fly Destination
   * Hidden Objects
   * Script
   * Current Map
   * Trades
   * Various Flags
   * Map Specific
   *  * Fossil
   *  * Vermillion Gym
   *  * Cinnabar Gym
   *  * Safari Zone
   *  * Daycare
*/
};

#endif // INDIVIDUALMAP_H
