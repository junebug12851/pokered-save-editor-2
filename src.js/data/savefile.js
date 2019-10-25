.pragma library

/**
   Copyright 2018 June Hanabi

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

/**
 * Provides means to work with a Pokemon Red save file and correctly extract
 * various data formats in a useable and readable form as well as write them
 * back correctly.
 */

.import "./savefile-expanded/saveFileIterator.js" as SaveFileIterator_
.import "./savefile-expanded/savefileExpanded.js" as SaveFileExpanded_
.import "./text.js" as Text
.import pse.gamedata 1.0 as GameData

const SaveFileIterator = SaveFileIterator_.SaveFileIterator;
const SaveFileExpanded = SaveFileExpanded_.SaveFileExpanded;
const text = Text.text;
const gameData = GameData.GameData.json;

class SaveFile
{
  constructor()
  {
    this.fileData = new Uint8Array(0x8000);
    this.fileDataExpanded = new SaveFileExpanded(this);

    // @TODO
    // Implement listening for onDataChange & onDataUpdate
  }

  get iterator() {
    return new SaveFileIterator(this);
  }

  // Sent from C++
  // Notifies internal savefile copy has been changed
  onDataChange(data, internalOnly = false) {
    this.fileData = data;

    if (!internalOnly)
      this.fileDataExpanded = new SaveFileExpanded(this);

    // @TODO
    // Implement this inward communication
  }

  // Sent from C++
  // Notifies that the internal savefile copy is requesting an update from
  // QML (This end)
  onDataUpdate() {
    // Write values back from the expanded file to this file
    this.fileDataExpanded.save(this);

    // Recalculate all checksums
    this.recalcChecksums();

    // @TODO
    // Implement this outbound communication
  }

  /*
   * bcd2number -> takes a UInt8Array with a BCD and returns the corresponding number.
   * input: UInt8Array
   * output: number
   * credit: joaomaia @ https://gist.github.com/joaomaia/3892692
  */
  bcd2number(bcd) {
    var n = 0;
    var m = 1;
    for (var i = 0; i < bcd.length; i += 1) {
      n += (bcd[bcd.length - 1 - i] & 0x0F) * m;
      n += ((bcd[bcd.length - 1 - i] >> 4) & 0x0F) * m * 10;
      m *= 100;
    }
    return n;
  }

  /*
   * number2bcd -> takes a number and returns the corresponding BCD in a UInt8Array.
   * input: 32 bit positive number, UInt8Array size
   * output: UInt8Array
   * credit: joaomaia @ https://gist.github.com/joaomaia/3892692
  */
  number2bcd(number, size) {
    var s = size || 4; //default value: 4
    var bcd = new Buffer(s);
    bcd.fill(0);
    while (number !== 0 && s !== 0) {
      s -= 1;
      bcd[s] = (number % 10);
      number = (number / 10) | 0;
      bcd[s] += (number % 10) << 4;
      number = (number / 10) | 0;
    }
    return bcd;
  }

  // Copies a range of bytes to a buffer of size and returns them
  getRange(from, size, reverse = false) {
    if (reverse)
      return this.fileData.subarray(from, from + size).reverse();
    else
      return this.fileData.subarray(from, from + size);
  }

  // Copies data to the save data at a particular place going no further
  // than the maximum size desired to be copied and the maximum array
  // given top copy from
  //
  // from = index to start copying at inclusive
  // size = maximum length to copy
  // data = array of data to copy into, will stop at size or data length
  copyRange(from, size, data, reverse = false) {
    if (reverse)
      data = data.reverse();

    for (let i = from, j = 0; j < data.length && i < (from + size); i++ , j++) {
      this.fileData[i] = data[j];
    }
  }

  // Gets a string from the save file auto-converted to normal characters
  // Strings are never reversed
  getStr(from, size, maxChars) {
    return text.convertFromCode(this.getRange(from, size), maxChars);
  }

  // Sets a string to save file auto-converted to in-game characters
  // Strings are never reversed
  setStr(from, size, maxChars, str) {
    this.copyRange(from, size, text.convertToCode(str, maxChars));
  }

  // Gets a value in hex from the save file
  getHex(from, size, prefix = false, reverse = false) {

    let rawHex = this.getRange(from, size, reverse);

    let hexStr = "";
    for (let i = 0; i < rawHex.length; i++) {
      hexStr += rawHex[i].toString(16);
    }
    hexStr = hexStr.toUpperCase();
    if (prefix)
      hexStr = `0x${hexStr}`;

    return hexStr;
  }

  // Saves a hex value to bytes in the save file
  setHex(from, size, hex, hasPrefix = false, reverse = false) {
    // Trim prefix if any
    if (hasPrefix)
      hex = hex.substr(2);

    // Convert to number
    let hexValue = parseInt(hex, 16);
    const hexValueOrig = hexValue;
    let hexArr = [];

    // Break apart number into seperate bytes and store them in an
    // array. This also places it big-endian style which is how the
    // save data is structured
    if (hexValue === 0)
      hexArr.push(0);
    else
      while (hexValue > 0) {
        hexArr.push(hexValue & 0xFF);
        hexValue >>= 8;
      }

    // Account for bug where 16-bit value under 0xFF still needs to take up
    // 16-bits
    if (hexValueOrig <= 0xFF && size == 2)
      hexArr.push(0x00);

    // Copy to save data
    this.copyRange(from, size, new Uint8Array(hexArr), !reverse);
  }

  // Get a Raw BCD value in integer format
  // BCD is always not reversed
  getBCD(from, size) {
    return this.bcd2number(this.getRange(from, size));
  }

  // Sets a number in raw BCD format
  // BCD is always little endian
  setBCD(from, size, val) {
    this.copyRange(from, size, this.number2bcd(val, size));
  }

  // Gets a bit from a value
  getBit(from, size, bit, reverse = false) {
    let value = this.getRange(from, size);

    if (reverse)
      value = value.reverse();

    let res = value[0];

    if (size > 0)
      for (let i = 1; i < size; i++) {
        res <<= 8;
        res |= value[i];
      }

    return (res & (1 << bit)) !== 0;
  }

  // Sets/Resets a bit in a value
  setBit(from, size, bit, val, reverse = false) {
    let value = this.getRange(from, size, reverse);

    let res = value[0];

    if (size > 0)
      for (let i = 1; i < size; i++) {
        res <<= 8;
        res |= value[i];
      }

    if (val)
      res |= (1 << bit);
    else
      res &= ~(1 << bit);

    const resHex = res.toString(16);
    this.setHex(from, size, resHex, false, reverse);
  }

  getWord(from, reverse = false) {
    let value = this.getRange(from, 2, reverse);

    let res = value[0];
    res <<= 8;
    res |= value[1];

    return res;
  }

  setWord(from, value, reverse = false) {

    const byteL = value & 0x00FF;
    const byteH = (value & 0xFF00) >> 8;

    this.copyRange(from, 2, new Uint8Array([byteH, byteL]), reverse);
  }

  getByte(offset) {
    return this.fileData[offset];
  }

  setByte(offset, value) {
    this.fileData[offset] = value;
  }

  // Calculates checksum from start index inclusive to end index exclusive
  // and returns it
  getChecksum(from, to) {
    const toChecksum = this.fileData.subarray(from, to);
    const checksum = new Uint8Array([0xFF]);;
    for (let i = 0; i < toChecksum.length; i++) {
      checksum[0] -= toChecksum[i];
    }
    return checksum[0];
  }

  // Recalculates all checksums and sets them on the save data
  // Skips Pokemon box checksums unless marked as formatted or unless forced
  recalcChecksums(force = false) {
    // Bank 1 Checksum
    this.fileData[0x3523] = this.getChecksum(0x2598, 0x3523);

    const boxesFormatted = (this.fileData[0x284C] & 0b10000000) > 0;
    if (boxesFormatted || force)
      this.recalcBoxesChecksums();
  }

  recalcBoxesChecksums() {
    // Bank 2 individual checksums for Boxes 1-6
    const bank2IndvChecksums = [
      this.getChecksum(0x4000, 0x4462),
      this.getChecksum(0x4462, 0x48C4),
      this.getChecksum(0x48C4, 0x4D26),
      this.getChecksum(0x4D26, 0x5188),
      this.getChecksum(0x5188, 0x55EA),
      this.getChecksum(0x55EA, 0x5A4C),
    ];
    this.copyRange(0x5A4D, 0x6, new Uint8Array(bank2IndvChecksums));

    // Bank 2 checksum excluding individual checksums
    // Similar to bank 1 checksum
    this.fileData[0x5A4C] = this.getChecksum(0x4000, 0x5A4C);

    // Bank 3 Checksums for Boxes 7-12
    const bank3IndvChecksums = [
      this.getChecksum(0x6000, 0x6462),
      this.getChecksum(0x6462, 0x68C4),
      this.getChecksum(0x68C4, 0x6D26),
      this.getChecksum(0x6D26, 0x7188),
      this.getChecksum(0x7188, 0x75EA),
      this.getChecksum(0x75EA, 0x7A4C),
    ];
    this.copyRange(0x7A4D, 0x6, new Uint8Array(bank3IndvChecksums));

    // Bank 3 Checksum
    this.fileData[0x7A4C] = this.getChecksum(0x6000, 0x7A4C);
  }
}

const saveFile = new SaveFile();
