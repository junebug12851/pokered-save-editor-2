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
#ifndef SAVEFILEITERATOR_H
#define SAVEFILEITERATOR_H

#include "../../common/types.h"
#include <QVector>

// To prevent cross-include and thus errors
class SaveFile;
class SaveFileToolset;

class SaveFileIterator
{
public:
  SaveFileIterator(SaveFile* saveFile);

  // Adjust position via absolute or relative offset
  SaveFileIterator* offsetTo(var16 val);
  SaveFileIterator* offsetBy(var16 val);
  SaveFileIterator* skipPadding(var16 val); // Alias for code cleanliness

  // Increment and Decrement by individual bytes
  SaveFileIterator* inc();
  SaveFileIterator* dec();

  // Get reference back to save file
  SaveFile* file();
  SaveFileToolset* toolset();

  // Bookmark your place and return to bookmark, works like a FIFO stack
  SaveFileIterator* push();
  SaveFileIterator* pop();

  // Here are all the specialized functions that auto-use the offset and
  // auto-increment the offset making things much easier

  QVector<var8> getRange(var16 size, var16 padding = 0, bool reverse = false);
  void copyRange(
      var16 size, QVector<var8> data, var16 padding = 0, bool reverse = false);
  QString getStr(var16 size, var8 maxLen, var16 padding = 0);
  void setStr(var16 size, var8 maxLen, QString str, var16 padding = 0);
  QString getHex(var16 size, var16 padding = 0, bool reverse = false);
  void setHex(var16 size, QString hex, var16 padding = 0, bool reverse = false);
  var32 getBCD(var8 size, var16 padding = 0);
  void setBCD(var8 size, var32 val, var16 padding = 0);
  bool getBit(var8 size, var8 bit, bool reverse = false);
  void setBit(
      var8 size, var8 bit, bool value, bool reverse = false);
  var16 getWord(var16 padding = 0, bool reverse = false);
  void setWord(var16 val, var16 padding = 0, bool reverse = false);
  var8 getByte(var16 padding = 0);
  void setByte(var8 val, var16 padding = 0);
  QVector<bool> getBitField(var16 size, var16 padding = 0);
  void setBitField(var16 size, QVector<bool> src, , var16 padding = 0);

  // Current Offset in Save File
  // Can be freely changed directly
  var16 offset = 0x0000;

protected:
  // Allows saving places in the save file and goin back to them
  QVector<var16>* state = new QVector<var16>();

  // The Save File
  SaveFile* saveFile = nullptr;
};

#endif // SAVEFILEITERATOR_H
