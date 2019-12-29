/*
  * Copyright 2019 June Hanabi
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
#ifndef FONTS_H
#define FONTS_H

#include "../common/types.h"
#include <QString>

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// There are 255 font options although most of them are "invalid" and thus
// use the tilemap.

struct FontEntry {
  FontEntry();

  // Optional values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  var8 ind; // Font Code
  QString name; // Font Output
  bool shorthand; // Is Font Output shorthand for longer output?
  bool picture; // Does this use the tilemap and not the font
  bool useTilemap; // ?? Perhaps same as picture (Forgot >_>)
  var8 length; // Font output length or potentially maximum length
  bool control; // Is this a control character
  bool multiChar; // Does this output a length greater than 1
  bool variable; // Does this output a variable
  bool singleChar; // Does this output a single char
  bool normal; // Would this be an in-game accessible font char
};

class Font
{
public:
  static void load();
  static QVector<FontEntry*>* font;
};

#endif // FONTS_H
