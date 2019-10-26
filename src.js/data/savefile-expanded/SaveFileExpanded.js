.pragma library

.import "./sections/Storage.js" as Storage_
.import "./sections/HoF.js" as HoF_
.import "./sections/Area/Area.js" as Area_
.import "./sections/Rival.js" as Rival_
.import "./sections/Player/Player.js" as Player_
.import "./sections/World/World.js" as World_
.import "./sections/Daycare.js" as Daycare_
.import "../savefile.js" as Savefile

const Storage = Storage_.Storage;
const HoF = Hof_.Hof;
const Area = Area_.Area;
const Rival = Rival_.Rival;
const Player = Player_.Player;
const World = World_.World;
const Daycare = Daycare_.Daycare;
const savefile = Savefile.saveFile;

class SaveFileExpanded {

  constructor(saveFile) {
    // Related to or equipped with the Player
    this.player = new Player();

    // Related to or equipped with the Rival
    this.rival = new Rival();

    // Related to the PC Storage System
    this.storage = new Storage();

    // Related to the current map the player is on and will be destroyed when
    // player leaves
    this.area = new Area();

    // Related to data pertaining to the state of the world or data pertaining
    // to certain maps that won't be destroyed and will be preserved when the
    // player leaves
    this.world = new World();

    // Related to the daycare
    this.daycare = new Daycare();

    // Related to the Hall of Fame
    this.hof = new HoF();

    if (saveFile !== undefined)
    this.load(saveFile);
  }

  load(saveFile) {
    this.player = new Player(saveFile);
    this.rival = new Rival(saveFile);
    this.storage = new Storage(saveFile);
    this.area = new Area(saveFile);
    this.world = new World(saveFile);
    this.daycare = new Daycare(saveFile);
    this.hof = new HoF(saveFile);
  }

  save(saveFile) {
    this.player.save(saveFile);
    this.rival.save(saveFile);
    this.storage.save(saveFile);
    this.area.save(saveFile);
    this.world.save(saveFile);
    this.daycare.save(saveFile);
    this.hof.save(saveFile);
  }
}
