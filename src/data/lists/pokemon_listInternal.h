/*
  * Copyright 2019 June Hanabi
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
#ifndef POKEMONNAMES_H
#define POKEMONNAMES_H

#include "../../common/types.h"

// Pokemon in internal order
// Not Pokedex ordering
// Internal ordering also inlcudes MissingNo and glitch Pokemon
enum class PokemonListInternal : var8 {
  RHYDON = 1,
  KANGASKHAN = 2,
  NIDORAN_M = 3,
  CLEFAIRY = 4,
  SPEAROW = 5,
  VOLTORB = 6,
  NIDOKING = 7,
  SLOWBRO = 8,
  IVYSAUR = 9,
  EXEGGUTOR = 10,
  LICKITUNG = 11,
  EXEGGCUTE = 12,
  GRIMER = 13,
  GENGAR = 14,
  NIDORAN_F = 15,
  NIDOQUEEN = 16,
  CUBONE = 17,
  RHYHORN = 18,
  LAPRAS = 19,
  ARCANINE = 20,
  MEW = 21,
  GYARADOS = 22,
  SHELLDER = 23,
  TENTACOOL = 24,
  GASTLY = 25,
  SCYTHER = 26,
  STARYU = 27,
  BLASTOISE = 28,
  PINSIR = 29,
  TANGELA = 30,
  MISSING_1_F = 31,
  MISSING_20 = 32,
  GROWLITHE = 33,
  ONIX = 34,
  FEAROW = 35,
  PIDGEY = 36,
  SLOWPOKE = 37,
  KADABRA = 38,
  GRAVELER = 39,
  CHANSEY = 40,
  MACHOKE = 41,
  MR_MIME = 42,
  HITMONLEE = 43,
  HITMONCHAN = 44,
  ARBOK = 45,
  PARASECT = 46,
  PSYDUCK = 47,
  DROWZEE = 48,
  GOLEM = 49,
  MISSING_32 = 50,
  MAGMAR = 51,
  MISSING_34 = 52,
  ELECTABUZZ = 53,
  MAGNETON = 54,
  KOFFING = 55,
  MISSING_38 = 56,
  MANKEY = 57,
  SEEL = 58,
  DIGLETT = 59,
  TAUROS = 60,
  MISSING_3_D = 61,
  MISSING_3_E = 62,
  MISSING_3_F = 63,
  FARFETCHD = 64,
  VENONAT = 65,
  DRAGONITE = 66,
  MISSING_43 = 67,
  MISSING_44 = 68,
  MISSING_45 = 69,
  DODUO = 70,
  POLIWAG = 71,
  JYNX = 72,
  MOLTRES = 73,
  ARTICUNO = 74,
  ZAPDOS = 75,
  DITTO = 76,
  MEOWTH = 77,
  KRABBY = 78,
  MISSING_4_F = 79,
  MISSING_50 = 80,
  MISSING_51 = 81,
  VULPIX = 82,
  NINETALES = 83,
  PIKACHU = 84,
  RAICHU = 85,
  MISSING_56 = 86,
  MISSING_57 = 87,
  DRATINI = 88,
  DRAGONAIR = 89,
  KABUTO = 90,
  KABUTOPS = 91,
  HORSEA = 92,
  SEADRA = 93,
  MISSING_5_E = 94,
  MISSING_5_F = 95,
  SANDSHREW = 96,
  SANDSLASH = 97,
  OMANYTE = 98,
  OMASTAR = 99,
  JIGGLYPUFF = 100,
  WIGGLYTUFF = 101,
  EEVEE = 102,
  FLAREON = 103,
  JOLTEON = 104,
  VAPOREON = 105,
  MACHOP = 106,
  ZUBAT = 107,
  EKANS = 108,
  PARAS = 109,
  POLIWHIRL = 110,
  POLIWRATH = 111,
  WEEDLE = 112,
  KAKUNA = 113,
  BEEDRILL = 114,
  MISSING_73 = 115,
  DODRIO = 116,
  PRIMEAPE = 117,
  DUGTRIO = 118,
  VENOMOTH = 119,
  DEWGONG = 120,
  MISSING_79 = 121,
  MISSING_7_A = 122,
  CATERPIE = 123,
  METAPOD = 124,
  BUTTERFREE = 125,
  MACHAMP = 126,
  MISSING_7_F = 127,
  GOLDUCK = 128,
  HYPNO = 129,
  GOLBAT = 130,
  MEWTWO = 131,
  SNORLAX = 132,
  MAGIKARP = 133,
  MISSING_86 = 134,
  MISSING_87 = 135,
  MUK = 136,
  MISSING_89 = 137,
  KINGLER = 138,
  CLOYSTER = 139,
  MISSING_8_C = 140,
  ELECTRODE = 141,
  CLEFABLE = 142,
  WEEZING = 143,
  PERSIAN = 144,
  MAROWAK = 145,
  MISSING_92 = 146,
  HAUNTER = 147,
  ABRA = 148,
  ALAKAZAM = 149,
  PIDGEOTTO = 150,
  PIDGEOT = 151,
  STARMIE = 152,
  BULBASAUR = 153,
  VENUSAUR = 154,
  TENTACRUEL = 155,
  MISSING_9_C = 156,
  GOLDEEN = 157,
  SEAKING = 158,
  MISSING_9_F = 159,
  MISSING_A_0 = 160,
  MISSING_A_1 = 161,
  MISSING_A_2 = 162,
  PONYTA = 163,
  RAPIDASH = 164,
  RATTATA = 165,
  RATICATE = 166,
  NIDORINO = 167,
  NIDORINA = 168,
  GEODUDE = 169,
  PORYGON = 170,
  AERODACTYL = 171,
  MISSING_AC = 172,
  MAGNEMITE = 173,
  MISSING_AE = 174,
  MISSING_AF = 175,
  CHARMANDER = 176,
  SQUIRTLE = 177,
  CHARMELEON = 178,
  WARTORTLE = 179,
  CHARIZARD = 180,
  MISSING_B_5 = 181,
  _KABUTOPS = 182,
  _AERODACTY = 183,
  GHOST = 184,
  ODDISH = 185,
  GLOOM = 186,
  VILEPLUME = 187,
  BELLSPROUT = 188,
  WEEPINBELL = 189,
  VICTREEBEL = 190,
};

#endif // POKEMONNAMES_H
