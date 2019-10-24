.pragma library

/**
   Copyright 2018 June Hanabi

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

.import "../../libs/lodash.js" as Lodash
.import pse.gamedata 1.0 as GameData

/*
 * This is a bit of a nightmare
 * It processes and indexes the raw Pokemon files
 * however it was poorly documented at the time of writing and now it's just
 * a cluster of code that's a nightmare to sift and sort through.
*/

const _ = Lodash._;
const gameData = GameData.GameData.json;

class PokemonDB
{
  constructor()
  {
    // Create class variables
    this.rawItems = [];
    this.rawMoves = [];
    this.rawPokemon = [];
    this.rawTypes = [];

    this.items = {};
    this.moves = {};
    this.pokemon = {};
    this.types = {};

    // Get Raw Data
    this.rawItems = JSON.parse(gameData("items"));
    this.rawMoves = JSON.parse(gameData("moves"));
    this.rawPokemon = JSON.parse(gameData("pokemon"));
    this.rawTypes = JSON.parse(gameData("types"));

    // Convert to index data by name in snake_case form
    this.process1(this.rawItems, this.items, "name");
    this.process1(this.rawMoves, this.moves, "name");
    this.process1(this.rawPokemon, this.pokemon, "name");
    this.process1(this.rawTypes, this.types, "name");

    // Add in aliases
    this.process1Alts(this.rawItems, this.items, "ind", "name");
    this.process1Alts(this.rawMoves, this.moves, "ind", "name");
    this.process1Alts(this.rawPokemon, this.pokemon, "ind", "name");
    this.process1Alts(this.rawTypes, this.types, "ind", "name");

    // Circular Link everything
    this.process2Items();
    this.process2Moves();
    this.process2Pokemon();
  }

  process1(from, to, key) {
    for(let i = 0; i < from.length; i++) {
      const entry = from[i];
      to[_.snakeCase(entry[key])] = _.cloneDeep(entry);
    }
  }

  process1Alts(from, to, key, origKey) {
    for(let i = 0; i < from.length; i++) {
      const entry = from[i];
      to[entry[key]] = to[_.snakeCase(entry[origKey])];
    }
  }

  process2Items() {
    const self = this;

    _.forOwn(this.items, function(value) {
      if(value.tm !== undefined && value._tm === undefined) {
        value._tm = value.tm;
        value.tm = _.find(self.moves, ['tm', value.tm]);
      }
      if(value.hm !== undefined && value._hm === undefined) {
        value._hm = value.hm;
        value.hm = _.find(self.moves, ['hm', value.hm]);
      }
    });
  }

  process2Moves() {
    const self = this;

    _.forOwn(this.moves, function(value) {

      if(value.type !== undefined && value._type === undefined) {
        value._type = value.type;
        value.type = _.find(self.types, ['name', _.startCase(_.lowerCase(value.type))]);
      }
      if(value.tm !== undefined && value._tm === undefined) {
        value._tm = value.tm;
        value.tm = _.find(self.items, ['_tm', value.tm]);
      }
      if(value.hm !== undefined && value._hm === undefined) {
        value._hm = value.hm;
        value.hm = _.find(self.items, ['_hm', value.hm]);
      }
    });
  }

  process2Pokemon() {
    const self = this;

    _.forOwn(this.pokemon, function(value) {
      if(value.moves !== undefined) {
        for(let i = 0; i < value.moves.length; i++) {
          const move = value.moves[i];
          if(move._move !== undefined)
            continue;

          move._move = move.move;
          move.move = _.find(self.moves, ['name', _.startCase(_.lowerCase(move.move))]);
        }
      }

      if(value.initial !== undefined && value._initial === undefined) {
        value._initial = _.cloneDeep(value.initial);
        for(let i = 0; i < value.initial.length; i++) {
          const initial = value.initial[i];
          value.initial[i] = _.find(self.moves, ['name', _.startCase(_.lowerCase(initial))]);
        }
      }

      if(value.tmHm !== undefined && value._tmHm === undefined) {
        value._tmHm = _.cloneDeep(value.tmHm);
        for(let i = 0; i < value.tmHm.length; i++) {
          const tmHmEntry = value.tmHm[i];
          value.tmHm[i] = _.find(self.moves, ['_tm', tmHmEntry]);
        }
      }

      if(value.type1 !== undefined && value._type1 === undefined) {
        value._type1 = value.type1;
        value.type1 = _.find(self.types, ['name', _.startCase(_.lowerCase(value.type1))]);
      }

      if(value.type2 !== undefined && value._type2 === undefined) {
        value._type2 = value.type2;
        value.type2 = _.find(self.types, ['name', _.startCase(_.lowerCase(value.type2))]);
      }

      if(value.evolution !== undefined) {
        const process = (entry) => {
          if(entry.item !== undefined && entry._item === undefined) {
            entry._item = entry.item;
            entry.item = _.find(self.items, ['name', _.startCase(_.lowerCase(entry.item))]);
          }

          if(entry._toName !== undefined)
          return;

          entry._toName = entry.toName;
          entry.toName = _.find(self.pokemon, ['name', _.startCase(_.lowerCase(entry.toName))]);
        }

        if(Array.isArray(value.evolution)) {
          for(let i = 0; i < value.evolution.length; i++) {
            process(value.evolution[i]);
          }
        }
        else
          process(value.evolution);
      }
    });
  }
}

const pokemonDB = new PokemonDB();
