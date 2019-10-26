.pragma library

.import "./fragments/HoFRecord.js" as HoFRecord_
const HoFRecord = HoFRecord_.HoFRecord;

class HoF {
  constructor(saveFile) {
    this.hallOfFame = [];

    if (saveFile !== undefined)
      this.load(saveFile );
  }

  load(saveFile) {
    const hofRecordCount = saveFile.getByte(0x284E);
    for (let i = 0; i < hofRecordCount && i < 50; i++) {
      this.hallOfFame.push(new HoFRecord(saveFile, i));
    }
  }

  save(saveFile) {
    saveFile.setByte(0x284E, this.hallOfFame.length);
    for (let i = 0; i < this.hallOfFame.length && i < 50; i++) {
      this.hallOfFame[i].save(saveFile, i);
    }
  }
}
