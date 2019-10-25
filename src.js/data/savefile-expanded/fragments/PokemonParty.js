.pragma library

.import "./PokemonBox.js" as PokemonBox_
const PokemonBox = PokemonBox_.PokemonBox;

class PokemonParty extends PokemonBox {
  constructor(saveFile,
              offset,
              nicknameStartOffset,
              otNameStartOffset,
              index) {

    // Due to an annoying attribute of typescript compiling variable
    // declarations below the super call thus resetting child class to default
    // after child and parent class have been initialized properly
    // we have to work around this annoyance by initiating a blank parent
    // and typescripts injected code for a blank child followed by the actual
    // loading process handled by the child and delegated manually to the parent
    // This annoyance took several hours to figure out why only the child
    // kept resetting to default values after loading properly
    super();

    this.maxHP = 0;
    this.attack = 0;
    this.defense = 0;
    this.speed = 0;
    this.special = 0;

    if (saveFile !== undefined)
      this.load(saveFile,
                offset,
                nicknameStartOffset,
                otNameStartOffset,
                index)
  }

  // Modifies a Pokemon Box to be a Pokemon Party and creates initial stats
  static convertToPokemonParty(pkmn) {
    const _pkmn = pkmn;
    Object.setPrototypeOf(_pkmn, PokemonParty.prototype);
    _pkmn.updateStats();
  }

  // Modifies a Pokemon Party to be a Pokemon Box
  static convertToPokemonBox(pkmn) {
    delete pkmn.maxHP;
    delete pkmn.attack;
    delete pkmn.defense;
    delete pkmn.speed;
    delete pkmn.special;
    Object.setPrototypeOf(pkmn, PokemonBox.prototype);
  }

  load(saveFile,
       offset,
       nicknameStartOffset,
       otNameStartOffset,
       index) {

    // Mark record size at end to the expanded size of 0x2C
    // Pokemon Party has expanded data
    const it = super.load(saveFile,
                          offset,
                          nicknameStartOffset,
                          otNameStartOffset,
                          index,
                          0x2C);

    this.level = it.getByte();
    this.maxHP = it.getWord();
    this.attack = it.getWord();
    this.defense = it.getWord();
    this.speed = it.getWord();
    this.special = it.getWord();

    return it;
  }

  save(saveFile,
       offset,
       speciesStartOffset,
       nicknameStartOffset,
       otNameStartOffset,
       index) {

    const it = super.save(saveFile,
                          offset,
                          speciesStartOffset,
                          nicknameStartOffset,
                          otNameStartOffset,
                          index,
                          0x2C);

    it.setByte(this.level);
    it.setWord(this.maxHP);
    it.setWord(this.attack);
    it.setWord(this.defense);
    it.setWord(this.speed);
    it.setWord(this.special);

    return it;
  }

  updateStats() {

    // Stop here if invalid Pokemon
    if(this.isValidPokemon === false)
      return;

    this.maxHP = this.hpStat;
    this.attack = this.attackStat;
    this.defense = this.defenseStat;
    this.speed = this.speedStat;
    this.special = this.specialStat;
  }
}
