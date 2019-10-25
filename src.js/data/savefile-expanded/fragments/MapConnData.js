.pragma library

class MapConnData {
  constructor(saveFile, offset) {
    // Connected Map
    this.map = "";

    // Pointer to upper left corner of map without adjustment for X position
    this.viewPtr = 0;

    // Strip
    this.stripSrc = 0;
    this.stripDest = 0;
    this.stripWidth = 0;

    // Strip Alignment
    this.yAlign = 0;
    this.xAlign = 0;

    if (saveFile !== undefined)
      this.load(saveFile, offset);
  }

  load(saveFile, offset) {
    const it = saveFile.iterator.offsetTo(offset);

    const mapPtr = it.getByte();
    this.stripSrc = it.getWord();
    this.stripDest = it.getWord();
    this.stripWidth = it.getByte();
    const width = it.getByte();
    this.yAlign = it.getByte();
    this.xAlign = it.getByte();
    this.viewPtr = it.getWord(0, true);

    this.map = `${mapPtr.toString(16).padStart(2, "0").toUpperCase()}_${width}`;
  }

  save(saveFile, offset) {
    const it = saveFile.iterator.offsetTo(offset);
    const map = this.map.split("_");

    it.setByte(parseInt(map[0], 16));
    it.setWord(this.stripSrc);
    it.setWord(this.stripDest);
    it.setByte(this.stripWidth);
    it.setByte(parseInt(map[1]));
    it.setByte(this.yAlign);
    it.setByte(this.xAlign);
    it.setWord(this.viewPtr, 0, true);
  }
}
