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
#include <QQmlEngine>
#include <pse-common/utility.h>

#include "./fontsdb.h"
#include "./util/gamedata.h"
#include "./util/fontsearch.h"
#include "./entries/fontdbentry.h"

void FontsDB::load()
{
  static bool once = false;
  if(once)
    return;

  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json("font");

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new FontDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  once = true;
}

void FontsDB::index()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert("ind"+QString::number(entry->ind), entry);
  }

  once = true;
}

void FontsDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
  for(auto el : store)
    el->qmlProtect(engine);
}

void FontsDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<FontsDB>("PSE.DB.FontsDB", 1, 0, "FontsDB", "Can't instantiate in QML");
  once = true;
}

FontsDB::FontsDB()
{
  qmlRegister();
  load();
}

FontSearch* FontsDB::searchRaw() const
{
  return new FontSearch();
}

const QScopedPointer<const FontSearch, QScopedPointerDeleteLater> FontsDB::search() const
{
  return QScopedPointer<const FontSearch, QScopedPointerDeleteLater>(
        new FontSearch());
}

// Converts a string filled with english typable in-game text code
// representations to raw in-game code
// If fed strings not in the representation list, the unknown characters will
// be ignored thus possibly corrupting output
// Possibly very slow
const QVector<int> FontsDB::convertToCode(QString str, int maxLen, const bool autoEnd) const
{
  // Code string to return
  QVector<int> code;

  // Last code
  int lastCode = 0;

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
    for (int i = 0; i < store.length(); i++) {

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
      str = str.mid(1);

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
const QString FontsDB::convertFromCode(const QVector<int> codes, const int maxLen) const
{
  // Prepare empty string
  QString eng = "";

  for (int i = 0; i < codes.length(); i++) {
    int code = codes[i];

    // Don't include the end terminator
    // stop here if there is one
    if (code == 0x50)
      break;

    // If the code is invalid then also stop here
    if(ind.value("ind"+QString::number(code), nullptr) == nullptr)
      continue;

    // Append to end of string the typable equivelant
    eng += ind.value("ind"+QString::number(code))->name;

    // Stop here if we've reached max length
    if (i >= maxLen)
      break;
  }

  // Return string
  return eng;
}

// Converts an english format string to code represented as how it would be
// in-game
const QString FontsDB::expandStr(const QString msg, const int maxLen, const QString rival, const QString player) const
{
  // Convert string to char codes
  // Very expensive
  QVector<int> charCodes = convertToCode(msg, maxLen, true);

  // First we go through and stop or react on some control codes
   for(int i = 0; i < charCodes.length(); i++) {
     // Grab a code
     int code = charCodes[i];

     // <page> => stop & end
     // Verified: It awaits for you to continue by pressing any key to advance
     // dialog
     if(code == 73) {
       charCodes = charCodes.mid(0, i);
       break;
     }

     // <cont_> => stop & end
     // Verified: It awaits for you to continue by pressing any key to advance
     // dialog.
     else if(code == 75) {
       charCodes = charCodes.mid(0, i);
       break;
     }

     // <autocont> => cut off up til now and keep rest
     // Verified: It doesn't await for you to continue and just advances
     // dialog.
     else if(code == 76) {
       charCodes = charCodes.mid(i+1, -1);
       i = 0;
     }

     // <end> => stop & end
     // Verified: Ends dialog
     else if(code == 80) {
       charCodes = charCodes.mid(0, i);
       break;
     }

     // <para> => stop & end
     // Verified: It awaits for you to continue by pressing any key to advance
     // dialog.
     else if(code == 81) {
       charCodes = charCodes.mid(0, i);
       break;
     }

     // <cont> => stop & end
     // Verified: It awaits for you to continue by pressing any key to advance
     // dialog.
     else if(code == 85) {
       charCodes = charCodes.mid(0, i);
       break;
     }

     // <done> => cut off up til now and keep rest
     // Verified: More or less does <autocont>
     else if(code == 87) {
       charCodes = charCodes.mid(i+1, -1);
     }

     // <prompt> => cut off up til now and keep rest
     // Verified: More or less does <para>
     else if(code == 88) {
       charCodes = charCodes.mid(0, i);
       break;
     }
   }

   int lineCount = 1;

  // Now begin processing it, we have to expand the expansive char codes first
  for(int i = 0; i < charCodes.length(); i++) {

    // Grab a code
    int code = charCodes[i];

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

    // <next> => move to next line
    // Verified: Moves down a line
    else if(code == 78) {
      lineCount++;
      if(lineCount > 2) { // Allow 1 past end of line
        charCodes = charCodes.mid(0, i);
        break;
      }
    }

    // <line> => move to next line
    // Verified: Moves down a line
    else if(code == 79) {
      lineCount++;
      if(lineCount > 2) { // Allow 1 past end of line
        charCodes = charCodes.mid(0, i);
        break;
      }
    }

    // <dex> => Add a period
    // Verified: Does different things, mostly just adds a period, sometimes
    // other stuff like restarting the line
    else if(code == 95) {
      splice(charCodes, ".", i);
    }
  }

  // Return as a converted string, also create line-breaks
  auto ret = convertFromCode(charCodes, 255);
  ret = ret.replace("<next>", "\n");
  ret = ret.replace("<line>", "\n");

  return ret;
}

FontsDB* FontsDB::inst()
{
  static FontsDB* _inst = new FontsDB();
  return _inst;
}

const QVector<FontDBEntry*> FontsDB::getStore() const
{
  return store;
}

const QHash<QString, FontDBEntry*> FontsDB::getInd() const
{
  return ind;
}

int FontsDB::getStoreSize() const
{
  return store.size();
}

const FontDBEntry* FontsDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

const FontDBEntry* FontsDB::getIndAt(const QString val) const
{
  return ind.value(val, nullptr);
}

const FontDBEntry* FontsDB::getStoreByVal(int ind) const
{
  // Font values start at 1, to get requested value, offset by 1
  // Check for negatives as well
  ind--;
  if(ind >= store.size() ||
     ind < 0)
    return nullptr;

  return store.at(ind);
}

int FontsDB::countSizeOf(const QString val) const
{
  // Ignore max size and ending byte
  return convertToCode(val, 255, false).size();
}

int FontsDB::countSizeOfExpanded(const QString val) const
{
  // Gives length of expanded string in tiles with maximum player and rival
  // names
  return convertToCode(
        expandStr(val, 255, "1234567", "1234567"), 255, false).size();
}

void FontsDB::splice(QVector<int>& out, const QString in, const int ind) const
{
  QVector<int> tmp = convertToCode(in, 100, false);
  out.remove(ind);
  for(int j = 0; j < tmp.length(); j++)
    out.insert(ind+j, tmp.at(j));
}
