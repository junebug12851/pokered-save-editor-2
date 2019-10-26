.pragma library

.import "../savefile.js" as Savefile
const savefile = Savefile.saveFile;

class SaveFileIterator {
  constructor(saveFile) {
    this.offset = 0x0000;
    this.saveFile = saveFile;
    this.state = [];
  }

  // Move offset to
  offsetTo(val) {
    this.offset = val;
    return this;
  }

  // Move offset by
  offsetBy(val) {
    this.offset += val;
    return this;
  }

  // Alias for above
  skipPadding(val) {
    return this.offsetBy(val);
  }

  inc() {
    this.offset++;
    return this;
  }

  dec() {
    this.offset--;
    return this;
  }

  get file() {
    return this.saveFile;
  }

  // Save and restore bookmarked offsets in FIFO ordering
  push() {
    this.state.push(this.offset);
    return this;
  }

  pop() {
    let val = this.state.pop();

    // In case of error (too many pops), revert offset to zero
    if (val === undefined)
      val = 0x0000;

    this.offset = val;
    return this;
  }

  //
  // Specialized copies of main savefile service
  //

  getRange(size, padding = 0, reverse = false) {
    const ret = this.file.getRange(this.offset, size, reverse);
    this.offsetBy(size + padding);
    return ret;
  }

  copyRange(size, data, padding = 0, reverse = false) {
    const ret = this.file.copyRange(this.offset, size, data, reverse);
    this.offsetBy(size + padding);
    return ret;
  }

  getStr(size, maxChars, padding = 0) {
    const ret = this.file.getStr(this.offset, size, maxChars);
    this.offsetBy(size + padding);
    return ret;
  }

  setStr(size, maxChars, str, padding = 0) {
    const ret = this.file.setStr(this.offset, size, maxChars, str);
    this.offsetBy(size + padding);
    return ret;
  }

  getHex(size, prefix, padding = 0, reverse = false) {
    const ret = this.file.getHex(this.offset, size, prefix, reverse);
    this.offsetBy(size + padding);
    return ret;
  }

  setHex(size, hex, hasPrefix, padding = 0, reverse = false) {
    const ret = this.file.setHex(this.offset, size, hex, hasPrefix, reverse);
    this.offsetBy(size + padding);
    return ret;
  }

  getBCD(size, padding = 0) {
    const ret = this.file.getBCD(this.offset, size);
    this.offsetBy(size + padding);
    return ret;
  }

  setBCD(size, val, padding = 0) {
    const ret = this.file.setBCD(this.offset, size, val);
    this.offsetBy(size + padding);
    return ret;
  }

  // We can't adjust for bit operations
  getBit(size, bit, reverse = false) {
    return this.file.getBit(this.offset, size, bit, reverse);
  }

  setBit(size, bit, val, reverse = false) {
    return this.file.setBit(this.offset, size, bit, val, reverse);
  }

  getWord(padding = 0, reverse = false) {
    const ret = this.file.getWord(this.offset, reverse);
    this.offsetBy(2 + padding);
    return ret;
  }

  setWord(value, padding = 0, reverse = false) {
    const ret = this.file.setWord(this.offset, value, reverse);
    this.offsetBy(2 + padding);
    return ret;
  }

  getByte(padding = 0) {
    const ret = this.file.getByte(this.offset);
    this.offsetBy(1 + padding);
    return ret;
  }

  setByte(value, padding = 0) {
    const ret = this.file.setByte(this.offset, value);
    this.offsetBy(1 + padding);
    return ret;
  }
}
