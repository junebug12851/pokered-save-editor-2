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
#include <QVector>
#include <QJsonArray>

#include "./fonts.h"
#include "./gamedata.h"
#include "./fontsearch.h"

FontDBEntry::FontDBEntry()
{
  shorthand = false;
  picture = false;
  useTilemap = false;
  control = false;
  multiChar = false;
  variable = false;
  singleChar = false;
  normal = false;
}

void FontsDB::load()
{
  // Grab Event Pokemon Data
  auto fontData = GameData::json("font");

  // Go through each event Pokemon
  for(QJsonValue fontEntry : fontData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new FontDBEntry();

    // Set simple properties
    entry->name = fontEntry["name"].toString();
    entry->ind = fontEntry["ind"].toDouble();
    entry->length = fontEntry["length"].toDouble();

    // Set simple optional properties
    if(fontEntry["shorthand"].isBool())
      entry->shorthand = fontEntry["shorthand"].toBool();

    if(fontEntry["picture"].isBool())
      entry->picture = fontEntry["picture"].toBool();

    if(fontEntry["useTilemap"].isBool())
      entry->useTilemap = fontEntry["useTilemap"].toBool();

    if(fontEntry["control"].isBool())
      entry->control = fontEntry["control"].toBool();

    if(fontEntry["multiChar"].isBool())
      entry->multiChar = fontEntry["multiChar"].toBool();

    if(fontEntry["variable"].isBool())
      entry->variable = fontEntry["variable"].toBool();

    if(fontEntry["singleChar"].isBool())
      entry->singleChar = fontEntry["singleChar"].toBool();

    if(fontEntry["normal"].isBool())
      entry->normal = fontEntry["normal"].toBool();

    // Add to array
    store.append(entry);
  }

  delete fontData;
}

void FontsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

FontSearch* FontsDB::search()
{
  return new FontSearch();
}

// Converts a string filled with english typable in-game text code
// representations to raw in-game code
// If fed strings not in the representation list, the unknown characters will
// be ignored thus possibly corrupting output
// Possibly very slow
QVector<var8> FontsDB::convertToCode(QString str, var8 maxLen, bool autoEnd)
{
  // Code string to return
  QVector<var8> code;

  // Last code
  var8 lastCode = 0;

  // Auto Ending takes up a character, remove 1 from max length if we auto-end
  if(autoEnd)
    maxLen -= 1;

  // loop through all string characters
  while (str.length() != 0) {

    bool match = false;

    // Loop through all 255 font characters
    // This is what makes it slow and why this is complicated
    // "a" maps to "a" in-game but "<m>" maps to "male symbol" in-game
    // "a<m>" maps to 2 characters in-game not 4. When we find a symbol we have
    // to remove all the characters in the string before proceeding.
    for (var8 i = 0; i < store.length(); i++) {

      // Find a starting match
      // Ignore this character if none are found
      FontDBEntry* transPair = store.at(i);
      if (!str.startsWith(transPair->name))
        continue;

      // Match was found
      match = true;

      // Slice off match from string start
      str = str.mid(transPair->name.length());

      // Append code to code array and set last code
      code.append(transPair->ind);
      lastCode = transPair->ind;

      // Break early
      break;
    }

    // If no match then strip unknown character and continue
    if (match == false)
      str = str.left(1);

    // Stop here if code array is at max bytes or a stop code was manually
    // set (0x50)
    if (code.length() >= maxLen || lastCode == 0x50)
      break;
  }

  // In-Game font marks the end-of-line to be 0x50
  // if auto-end is requested, append an 0x50
  if(autoEnd)
    code.append(0x50);

  // Return finished product
  return code;
}

// Much easier and faster, just expand the in-game code to it's english
// representation directly
QString FontsDB::convertFromCode(QVector<var8> codes, var8 maxLen)
{
  // Prepare empty string
  QString eng = "";

  for (var8 i = 0; i < codes.length(); i++) {
    var8 code = codes[i];

    // Don't include the end terminator
    // stop here if there is one
    if (code == 0x50)
      break;

    // If the code is invalid then also stop here
    if(ind.value(QString::number(code), nullptr) == nullptr)
      continue;

    // Append to end of string the typable equivelant
    eng += ind.value(QString::number(code))->name;

    // Stop here if we've reached max length
    if (i >= maxLen)
      break;
  }

  // Return string
  return eng;
}

// Converts an english format string to code represented as how it would be
// in-game
QString FontsDB::expandStr(QString msg, var8 maxLen, QString rival, QString player)
{
  // Convert string to char codes
  // Very expensive
  QVector<var8> charCodes = convertToCode(msg, maxLen, false);

  // Now begin processing it, we have to expand the expansive char codes first
  for(var8 i = 0; i < charCodes.length(); i++) {

    // Grab a code
    var8 code = charCodes[i];

    // Expand if any of the codes are present

    // <pkmn> => <pk><mn>
    if (code == 0x4A) {
      splice(charCodes, "<pk><mn>", i);
    }
    // <player> => RED
    else if (code == 0x52) {
      splice(charCodes, player, i);
    }
    // <rival> => BLUE
    else if (code == 0x53) {
      splice(charCodes, rival, i);
    }
    // <poke> => POK<e>
    else if (code == 0x54) {
      splice(charCodes, "POK<e>", i);
    }
    // <......> => <...><...>
    else if (code == 0x56) {
      splice(charCodes, "<...><...>", i);
    }
    // <targ> => CHARIZARD
    else if (code == 0x59) {
      splice(charCodes, "CHARIZARD", i);
    }
    // <user> => Enemy BLASTOISE
    else if (code == 0x5A) {
      splice(charCodes, "Enemy BLASTOISE", i);
    }
    // <pc> -> PC
    else if (code == 0x5B) {
      splice(charCodes, "PC", i);
    }
    // <tm> -> TM
    else if (code == 0x5C) {
      splice(charCodes, "TM", i);
    }
    // <trainer> => TRAINER
    else if (code == 0x5D) {
     splice(charCodes, "TRAINER", i);
    }
    // <rocket>
    else if (code == 0x5E) {
      splice(charCodes, "ROCKET", i);
    }
  }

  // Return as a converted string
  return convertFromCode(charCodes, 255);
}

void FontsDB::splice(QVector<var8>& out, QString in, var8 ind)
{
  QVector<var8> tmp = convertToCode(in, 100, false);
  out.remove(ind);
  for(var8 j = 0; j < tmp.length(); j++)
    out.insert(ind+j, tmp.at(j));
}

QVector<FontDBEntry*> FontsDB::store = QVector<FontDBEntry*>();
QHash<QString, FontDBEntry*> FontsDB::ind = QHash<QString, FontDBEntry*>();
