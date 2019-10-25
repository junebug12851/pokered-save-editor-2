.pragma library

class HoFPokemon {
  constructor(saveFile, recordOffset, index) {
    /**
     * Record Data
    */
    this.species = 0;
    this.level = 0;
    this.name = "";

    if (saveFile !== undefined)
      this.load(
        saveFile,
        recordOffset,
        index
        );
  }

  load(saveFile, recordOffset, index) {

    // Calculate Pokemon Offset in the record
    // Records are 0x10 in size
    // Multiply record number with 0x10 (Record Size) and add to offset
    // record start
    const pokemonOffset = (0x10 * index) + recordOffset;

    /**
         * Record Data
         */

    // Extract Pokemon Data
    this.species = saveFile.getByte(pokemonOffset + 0);
    this.level = saveFile.getByte(pokemonOffset + 1);
    this.name = saveFile.getStr(pokemonOffset + 2, 0xB, 10);
  }

  save(saveFile, recordOffset, index) {
    const pokemonOffset = (0x10 * index) + recordOffset;

    saveFile.setByte(pokemonOffset + 0, this.species);
    saveFile.setByte(pokemonOffset + 1, this.level);
    saveFile.setStr(pokemonOffset + 2, 0xB, 10, this.name);

    // Normally the Gameboy will actively zero out padding bytes
    // but we don't set padding bytes per the strict "Only touch whats needed" rule
  }
}
