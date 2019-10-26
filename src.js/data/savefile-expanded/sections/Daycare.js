.pragma library

.import "./fragments/PokemonBox.js" as PokemonBox_
const PokemonBox = PokemonBox_.PokemonBox;

class Daycare {
  constructor(saveFile) {
    this.dayCare = null;

    if (saveFile !== undefined)
      this.load(saveFile);
  }

  load(saveFile) {
    // Is the daycare in use, if so extract daycare Pokemon Information
    if (saveFile.getByte(0x2CF4) > 0)
      this.dayCare = new PokemonBox(saveFile, 0x2D0B, 0x2CF5, 0x2D00, 0);
  }

  save(saveFile) {
    saveFile.setByte(0x2CF4, (this.dayCare != null) ? 1 : 0);
    if (this.dayCare != null)
      this.dayCare.save(saveFile, 0x2D0B, null, 0x2CF5, 0x2D00, 0);
  }
}
