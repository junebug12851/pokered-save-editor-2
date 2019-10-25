.pragma library

class SignData {
  constructor(savefile, index) {
    this.y = 0;
    this.x = 0;
    this.text = 0;

    if (savefile !== undefined)
      this.load(savefile, index);
  }

  load(savefile, index) {
    const it = savefile.iterator.offsetTo((2 * index) + 0x275D);
    this.y = it.getByte();
    this.x = it.getByte();

    it.offsetTo((1 * index) + 0x277D);
    this.text = it.getByte();
  }

  save(saveFile, index) {
    const it = saveFile.iterator.offsetTo((2 * index) + 0x275D);
    it.setByte(this.y);
    it.setByte(this.x);

    it.offsetTo((1 * index) + 0x277D);
    it.setByte(this.text);
  }
}
