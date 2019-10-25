.pragma library

class WarpData {
  constructor(savefile, index) {
    this.y = 0;
    this.x = 0;
    this.destWarp = 0;
    this.destMap = 0;

    if (savefile !== undefined)
      this.load(savefile, index);
  }

  load(savefile, index) {
    const it = savefile.iterator.offsetTo((0x4 * index) + 0x265B);

    this.y = it.getByte();
    this.x = it.getByte();
    this.destWarp = it.getByte();
    this.destMap = it.getByte();
  }

  save(savefile, index) {
    const it = savefile.iterator.offsetTo((0x4 * index) + 0x265B);

    it.setByte(this.y);
    it.setByte(this.x);
    it.setByte(this.destWarp);
    it.setByte(this.destMap);
  }
}
