.pragma library

class Rival {
  constructor(saveFile) {
    this.rivalName = "";
    this.rivalStarter = "";

    if (saveFile !== undefined)
      this.load(saveFile);
  }

  load(saveFile) {
    this.rivalName = saveFile.getStr(0x25F6, 0xB, 7);
    this.rivalStarter = saveFile.getHex(0x29C1, 1).toUpperCase().padStart(2, "0");
  }

  save(saveFile) {
    saveFile.setStr(0x25F6, 0xB, 7, this.rivalName);
    saveFile.setHex(0x29C1, 1, this.rivalStarter);
  }
}
