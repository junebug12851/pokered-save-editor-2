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

#include <QMetaType>
#include <QString>
#include <QHash>
#include <QVector>
#include <QJsonValue>

#include "../../common/types.h"

class FontSearch;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// There are 255 font options although most of them are "invalid" and thus
// use the tilemap.

struct FontDBEntry {
  FontDBEntry();
  FontDBEntry(QJsonValue& data);

  // Optional values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  var8 ind; // Font Code
  QString name; // Font Output
  bool shorthand = false; // Is Font Output shorthand for longer output?
  bool picture = false; // Does this use the tilemap and not the font
  bool useTilemap = false; // ?? Perhaps same as picture (Forgot >_>)
  var8 length; // Font output length or potentially maximum length
  bool control = false; // Is this a control character
  bool multiChar = false; // Does this output a length greater than 1
  bool variable = false; // Does this output a variable
  bool singleChar = false; // Does this output a single char
  bool normal = false; // Would this be an in-game accessible font char
};

Q_DECLARE_METATYPE(FontDBEntry)

class FontsDB
{
public:
  static void load();
  static void index();

  // Pull up a finder to narrow down fonts based on criteria
  // You must delete this when done
  // If you want to change parameters or startover you must obtain a new one
  // everytime
  static FontSearch* search();

  // Converts a string to in-game font characters
  // The string represents font characters, so special characters like
  // <m> or <pic01> also apply allowing for all 255 font chracters to be
  // leveraged with a simple keyboard
  // Very complex and expensive operation
  static QVector<var8> convertToCode(
      QString str, var8 maxLen = 11, bool autoEnd = true);

  // The opposite, converts a series of codes to a string
  // Very simple and fast operation
  static QString convertFromCode(QVector<var8> codes, var8 maxLen = 11);

  // Font characters also include variables such as "<player>" to insert player
  // name. The above method converts to and from "<player>" and code but
  // this expands the string to include the players name spelled out.
  // Use this to give a sort of "preview" as to how it would look in-game.
  // It's complex and expensive
  static QString expandStr(
      QString msg, var8 maxLen, QString rival = "BLUE", QString player = "RED");

  static QVector<FontDBEntry*> store;
  static QHash<QString, FontDBEntry*> ind;

private:
  static void splice(QVector<var8>& out, QString in, var8 ind);
};

Q_DECLARE_METATYPE(FontsDB)

#endif // FONTS_H
