/*
  * Copyright 2019 Fairy Fox
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
#pragma once
#include <pse-common/types.h>
#include <QVector>
#include "savefile_autoport.h"

// To prevent cross-include and thus errors
class SaveFile;
class SaveFileToolset;

/**
 * @brief A moving cursor over a SaveFile, layering auto-advancing reads/writes on
 *        top of SaveFileToolset.
 *
 * The expansion (parse) and flattening (write-back) walk the save sequentially.
 * Doing that with raw addresses is error-prone, so the iterator keeps a current
 * `offset` and offers the same primitives as SaveFileToolset but address-free:
 * each call reads/writes at `offset` and advances it by the size consumed (plus
 * optional `padding`). A small push()/pop() stack lets code bookmark a position,
 * wander off to read elsewhere, then return.
 *
 * Obtain one from `SaveFile::iterator()`. The caller owns it and must delete it
 * (noted on SaveFile::iterator()).
 *
 * @see SaveFileToolset for the underlying address-based primitives.
 */
class SAVEFILE_AUTOPORT SaveFileIterator
{
public:
  /// @param saveFile The save this cursor walks.
  SaveFileIterator(SaveFile* saveFile);

  /// Move the cursor to an absolute offset. Returns @c this for chaining.
  SaveFileIterator* offsetTo(var16 val);
  /// Move the cursor by a relative amount. Returns @c this for chaining.
  SaveFileIterator* offsetBy(var16 val);
  SaveFileIterator* skipPadding(var16 val); ///< Alias for code cleanliness (advance past padding).

  SaveFileIterator* inc(); ///< Increment the offset by one byte. Returns @c this.
  SaveFileIterator* dec(); ///< Decrement the offset by one byte. Returns @c this.

  SaveFile* file();         ///< Get reference back to the save file.
  SaveFileToolset* toolset(); ///< Get the underlying address-based toolset.

  /// Bookmark the current offset (push) ...
  SaveFileIterator* push();
  /// ... and return to the most recent bookmark (pop). Works like a FIFO stack.
  SaveFileIterator* pop();

  // Here are all the specialized functions that auto-use the offset and
  // auto-increment the offset making things much easier

  /// @copydoc SaveFileToolset::getRange Reads at the cursor, then advances by size + @p padding.
  QVector<var8> getRange(var16 size, var16 padding = 0, bool reverse = false);
  /// Writes at the cursor, then advances by size + @p padding.
  void copyRange(
      var16 size, QVector<var8> data, var16 padding = 0, bool reverse = false);
  QString getStr(var16 size, var8 maxLen, var16 padding = 0);   ///< Read a font-encoded string at the cursor; advances.
  void setStr(var16 size, var8 maxLen, QString str, var16 padding = 0); ///< Write a font-encoded string at the cursor; advances.
  QString getHex(var16 size, var16 padding = 0, bool reverse = false);  ///< Read hex at the cursor; advances.
  void setHex(var16 size, QString hex, var16 padding = 0, bool reverse = false); ///< Write hex at the cursor; advances.
  var32 getBCD(var8 size, var16 padding = 0);                  ///< Read a BCD number at the cursor; advances.
  void setBCD(var8 size, var32 val, var16 padding = 0);        ///< Write a BCD number at the cursor; advances.
  bool getBit(var8 size, var8 bit, bool reverse = false);      ///< Test a bit at the cursor.
  void setBit(
      var8 size, var8 bit, bool value, bool reverse = false);  ///< Set a bit at the cursor.
  var16 getWord(var16 padding = 0, bool reverse = false);      ///< Read a 16-bit word at the cursor; advances.
  void setWord(var16 val, var16 padding = 0, bool reverse = false); ///< Write a 16-bit word at the cursor; advances.
  var8 getByte(var16 padding = 0);                             ///< Read a byte at the cursor; advances.
  void setByte(var8 val, var16 padding = 0);                   ///< Write a byte at the cursor; advances.
  QVector<bool> getBitField(var16 size, var16 padding = 0);    ///< Read a bitfield at the cursor; advances.
  void setBitField(var16 size, QVector<bool> src, var16 padding = 0); ///< Write a bitfield at the cursor; advances.

  /// Current offset in the save file. Can be freely changed directly.
  var16 offset = 0x0000;

protected:
  /// Bookmark stack -- saves places in the save file to go back to (push/pop).
  QVector<var16> state;

  SaveFile* saveFile = nullptr; ///< The save file this cursor walks.
};
