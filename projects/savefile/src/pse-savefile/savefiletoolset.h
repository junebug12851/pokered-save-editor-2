/*
  * Copyright 2019 Twilight
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
#include <QString>
#include "savefile_autoport.h"

class SaveFile;

/**
 * @brief Low-level read/write primitives over a SaveFile's raw 32 KB buffer.
 *
 * Every byte the editor touches goes through here. The toolset speaks the Game
 * Boy save's actual encodings -- BCD numbers, in-game font strings, packed
 * bit-fields, big/little-endian words, and the Gen 1 checksum -- so higher layers
 * (the `expanded/*` objects) can ask for "the money at this address" instead of
 * doing byte math. Addresses are absolute offsets into the raw save.
 *
 * It holds a back-pointer to its owning SaveFile and reads/writes that file's
 * `data` directly; it does not own the buffer. For cursor-style sequential access
 * with auto-advancing offsets, see SaveFileIterator, which wraps this.
 *
 * @note `reverse` flags exist because some fields are stored in the opposite byte
 *       order; pass `reverse = true` to read/write them swapped.
 */
// Tools to operate on raw save file data
class SAVEFILE_AUTOPORT SaveFileToolset
{
public:
  /// @param newSaveFile The save whose raw `data` this toolset operates on.
  SaveFileToolset(SaveFile* newSaveFile);

  /**
   * bcd2number -> takes an array with a BCD and returns the corresponding number.
   * input: array
   * output: number
   * credit: joaomaia @ https://gist.github.com/joaomaia/3892692
   *
   * @param val Bytes holding a binary-coded-decimal value.
   * @return The decoded base-10 number.
  */
  var32 bcd2int(QVector<var8> val);

  /**
   * number2bcd -> takes a number and returns the corresponding BCD in an array
   * input: 16 bit positive number, array size (How many bytes)
   * output: array
   * credit: joaomaia @ https://gist.github.com/joaomaia/3892692
   *
   * @param number The value to encode.
   * @param size   Output width in bytes.
   * @return @p number encoded as binary-coded decimal.
  */
  QVector<var8> int2bcd(var32 number, var8 size = 2);

  /// Copies a range of bytes to a buffer of size and returns them.
  /// @param from Start offset (inclusive). @param size Byte count.
  /// @param reverse Read the range in reversed byte order.
  QVector<var8> getRange(var16 from, var16 size, bool reverse = false);

  /**
   * @brief Copies data into the save at a particular place, going no further
   *        than the maximum size desired and/or the source array length.
   * @param addr    index to start copying at, inclusive
   * @param size    maximum length to copy
   * @param data    array of data to copy into, will stop at size or data length
   * @param reverse reverse copies the data into the specified location
   */
  void copyRange(var16 addr, var16 size, QVector<var8> data, bool reverse = false);

  /// Gets a string from the sav file, converted from in-game font encoding
  /// to UTF-8 for easy reading and manipulation.
  /// @param addr Start offset. @param size Field byte size. @param maxLen Max characters.
  QString getStr(var16 addr, var16 size, var8 maxLen);

  /// Sets a string to the sav file, converted from UTF-8 to in-game font encoding.
  /// @param addr Start offset. @param size Field byte size. @param maxLen Max characters.
  /// @param str  The UTF-8 text to store.
  void setStr(var16 addr, var16 size, var8 maxLen, QString str);

  /// Gets a value in hex from the save file.
  QString getHex(var16 addr, var16 size, bool reverse = false);

  /// Saves a hex value to bytes in the save file.
  void setHex(var16 addr, var16 size, QString hex, bool reverse = false);

  /// Get a raw BCD value as a number.
  /// Casino coins are 16-bit BCD meaning the maximum is 9,999.
  /// Player money is 24-bit BCD meaning the maximum is 999,999.
  var32 getBCD(var16 addr, var8 size);

  /// Set a raw BCD val from a given number.
  /// Casino coins are 16-bit BCD meaning the maximum is 9,999.
  /// Player money is 24-bit BCD meaning the maximum is 999,999.
  void setBCD(var16 addr, var8 size, var32 val);

  /// Get a bit from a value.
  /// Here for conv. but it's actually simpler just to test it yourself given
  /// this is so complicated and expensive for bit testing.
  /// This method can only bit test a bit up to 4 bytes.
  bool getBit(var16 addr, var8 size, var8 bit, bool reverse = false);

  /// Set a bit into a value.
  /// Here for conv. but it's actually simpler just to set it yourself given
  /// this is so complicated and expensive for bit setting.
  /// This method can only bit set a bit up to 4 bytes.
  void setBit(var16 addr, var8 size, var8 bit, bool value, bool reverse = false);

  /// Get a 16-bit value.  `0x12,0x34 <=> 0x1234`.  If you want the reverse then reverse it.
  var16 getWord(var16 addr, bool reverse = false);
  /// Set a 16-bit value.  `0x12,0x34 <=> 0x1234`.  If you want the reverse then reverse it.
  void setWord(var16 addr, var16 val, bool reverse = false);

  /// Simply gets a byte.
  var8 getByte(var16 addr);
  /// Simply sets a byte.
  void setByte(var16 addr, var8 val);

  /// Gets an entire bitfield as a vector of bools.
  QVector<bool> getBitField(var16 addr, var16 size);
  /// Sets an entire bitfield from a vector of bools.
  void setBitField(var16 addr, var16 size, QVector<bool> src);

  /// Gets a single checksum from a given range.
  /// This properly calculates the checksum that Gen 1 games expect.
  var8 getChecksum(var16 addr, var16 size);

  /// Recalculates the checksum for all the boxes.
  /// This only needs to be called when the boxes checksums are used.
  /// The game doesn't always use the box checksums or calculate them.
  /// Use recalcChecksums below to have them calculated as appropriate.
  void recalcBoxesChecksums();

  /**
   * @brief Recalculates all checksums in all banks, following the strict rule of
   *        only calculating what's needed and when.
   *
   * If the game will never calculate the box checksums then neither will we --
   * there are cases when it won't. This also calculates bank 1, thus covering all
   * checksums in the file. Part of honouring byte-exact fidelity: we don't write
   * checksums the real game wouldn't.
   * @param force Recalculate even the conditionally-skipped checksums.
   */
  void recalcChecksums(bool force = false);

protected:
  SaveFile* saveFile; ///< Owning save; its raw `data` is what every method here reads/writes.
};
