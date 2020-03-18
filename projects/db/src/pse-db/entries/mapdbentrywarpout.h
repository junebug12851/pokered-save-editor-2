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
#ifndef MAPDBENTRYWARPOUT_H
#define MAPDBENTRYWARPOUT_H


// List of Warps on Map that warp out to a different map
// They can only warp to a "warp-in" point
struct DB_AUTOPORT MapDBEntryWarpOut
{
  MapDBEntryWarpOut();
  MapDBEntryWarpOut(QJsonValue& data, MapDBEntry* parent);
  void deepLink();

  // X & Y location on Map
  var8 x = 0;
  var8 y = 0;

  // Which pre-defined warp-in to warp to
  var8 warp = 0;

  // Which map to warp to
  QString map;

  // Is this warp-out not intended to be used
  bool glitch = false;

  // Go to map
  MapDBEntry* toMap = nullptr;
  MapDBEntry* parent = nullptr;

  // Go to warp spot on destination map
  MapDBEntryWarpIn* toWarp = nullptr;
};

#endif // MAPDBENTRYWARPOUT_H
