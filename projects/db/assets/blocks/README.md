# Map block data

Imported **verbatim** from the pret/pokered disassembly by `scripts/import_map_blocks.ps1`.
Do not hand-edit -- re-run the script (`-Check` validates without writing). It re-derives
and re-validates every byte on each run.

* `maps/<mapId>.blk` -- one byte per block, `width * height` bytes, row-major.
  `<mapId>` is the id the save stores in `wCurMap` (0x260A) and the `ind` in `maps.json`.
* `tilesets/<tilesetId>.bst` -- one block per 16 bytes (4x4 tile ids, row-major).
  `<tilesetId>` is the id in `wCurMapTileset` and the `ind` in `tileset.json`.
  Alias tilesets share a blockset, so several of these files are byte-identical copies.

Ids come from `data/maps/map_header_pointers.asm` (the authoritative id -> header table),
not from file names: several maps share one `.blk` through label aliases, and
`UndergroundPathRoute7Copy` has no `.blk` of its own.

## Glitch ids: imported, or not

`maps.json` marks 22 ids as unused glitch maps and gives them **no width or
height**. The ROM is less tidy: its header table quietly points those ids at a real map's
header, so a Game Boy would happily render one. We do not import them -- a map the editor's
own DB cannot size is a map it has no business drawing, and inventing the dimensions is not
ours to do. The map screen says so plainly instead.

* id 11 (maps.json: 'Unused Map 0B', glitch) -- ROM would load SaffronCity's header (20x18)
* id 105 (maps.json: 'Unused Map 69', glitch) -- ROM would load LancesRoom's header (13x13)
* id 106 (maps.json: 'Unused Map 6A', glitch) -- ROM would load LancesRoom's header (13x13)
* id 107 (maps.json: 'Unused Map 6B', glitch) -- ROM would load LancesRoom's header (13x13)
* id 109 (maps.json: 'Unused Map 6D', glitch) -- ROM would load LancesRoom's header (13x13)
* id 110 (maps.json: 'Unused Map 6E', glitch) -- ROM would load LancesRoom's header (13x13)
* id 111 (maps.json: 'Unused Map 6F', glitch) -- ROM would load LancesRoom's header (13x13)
* id 112 (maps.json: 'Unused Map 70', glitch) -- ROM would load LancesRoom's header (13x13)
* id 114 (maps.json: 'Unused Map 72', glitch) -- ROM would load LancesRoom's header (13x13)
* id 115 (maps.json: 'Unused Map 73', glitch) -- ROM would load LancesRoom's header (13x13)
* id 116 (maps.json: 'Unused Map 74', glitch) -- ROM would load LancesRoom's header (13x13)
* id 117 (maps.json: 'Unused Map 75', glitch) -- ROM would load LancesRoom's header (13x13)
* id 204 (maps.json: 'Unused Map CC', glitch) -- ROM would load RocketHideoutElevator's header (3x4)
* id 205 (maps.json: 'Unused Map CD', glitch) -- ROM would load RocketHideoutElevator's header (3x4)
* id 206 (maps.json: 'Unused Map CE', glitch) -- ROM would load RocketHideoutElevator's header (3x4)
* id 231 (maps.json: 'Unused Map E7', glitch) -- ROM would load Route16Gate1F's header (4x7)
* id 237 (maps.json: 'Unused Map ED', glitch) -- ROM would load SilphCo2F's header (15x9)
* id 238 (maps.json: 'Unused Map EE', glitch) -- ROM would load SilphCo2F's header (15x9)
* id 241 (maps.json: 'Unused Map F1', glitch) -- ROM would load SilphCo2F's header (15x9)
* id 242 (maps.json: 'Unused Map F2', glitch) -- ROM would load SilphCo2F's header (15x9)
* id 243 (maps.json: 'Unused Map F3', glitch) -- ROM would load SilphCo2F's header (15x9)
* id 244 (maps.json: 'Unused Map F4', glitch) -- ROM would load SilphCo2F's header (15x9)

(The reachable `*_COPY` ids are a different thing -- `maps.json` sizes those, and they import
normally.)

## ROM overruns (reproduced, not patched)

The game copies `width * height` block bytes no matter how long the `.blk` actually is,
so a short one runs on into the next map's blocks in ROM. That is real, shipped behaviour
and it is what these files contain:

* id 119 (UndergroundPathNorthSouth): UndergroundPathNorthSouth.blk is 92 bytes but the header says 4x24 = 96 -- 4 byte(s) read on into the next blocks in ROM (as the game does)

## Tilesets

| id | tileset | blockset | blocks |
|----|---------|----------|--------|
| 0 | OVERWORLD | `overworld.bst` | 128 |
| 1 | REDS_HOUSE_1 | `reds_house.bst` | 19 |
| 2 | MART | `pokecenter.bst` | 37 |
| 3 | FOREST | `forest.bst` | 128 |
| 4 | REDS_HOUSE_2 | `reds_house.bst` | 19 |
| 5 | DOJO | `gym.bst` | 116 |
| 6 | POKECENTER | `pokecenter.bst` | 37 |
| 7 | GYM | `gym.bst` | 116 |
| 8 | HOUSE | `house.bst` | 35 |
| 9 | FOREST_GATE | `gate.bst` | 128 |
| 10 | MUSEUM | `gate.bst` | 128 |
| 11 | UNDERGROUND | `underground.bst` | 17 |
| 12 | GATE | `gate.bst` | 128 |
| 13 | SHIP | `ship.bst` | 62 |
| 14 | SHIP_PORT | `ship_port.bst` | 23 |
| 15 | CEMETERY | `cemetery.bst` | 110 |
| 16 | INTERIOR | `interior.bst` | 58 |
| 17 | CAVERN | `cavern.bst` | 128 |
| 18 | LOBBY | `lobby.bst` | 79 |
| 19 | MANSION | `mansion.bst` | 72 |
| 20 | LAB | `lab.bst` | 58 |
| 21 | CLUB | `club.bst` | 36 |
| 22 | FACILITY | `facility.bst` | 128 |
| 23 | PLATEAU | `plateau.bst` | 73 |

## Maps

| id | map | tileset | source | blocks (w x h) |
|----|-----|---------|--------|----------------|
| 0 | Pallet Town | OVERWORLD | `maps/PalletTown.blk` | 10 x 9 |
| 1 | Viridian City | OVERWORLD | `maps/ViridianCity.blk` | 20 x 18 |
| 2 | Pewter City | OVERWORLD | `maps/PewterCity.blk` | 20 x 18 |
| 3 | Cerulean City | OVERWORLD | `maps/CeruleanCity.blk` | 20 x 18 |
| 4 | Lavender Town | OVERWORLD | `maps/LavenderTown.blk` | 10 x 9 |
| 5 | Vermilion City | OVERWORLD | `maps/VermilionCity.blk` | 20 x 18 |
| 6 | Celadon City | OVERWORLD | `maps/CeladonCity.blk` | 25 x 18 |
| 7 | Fuchsia City | OVERWORLD | `maps/FuchsiaCity.blk` | 20 x 18 |
| 8 | Cinnabar Island | OVERWORLD | `maps/CinnabarIsland.blk` | 10 x 9 |
| 9 | Indigo Plateau | PLATEAU | `maps/IndigoPlateau.blk` | 10 x 9 |
| 10 | Saffron City | OVERWORLD | `maps/SaffronCity.blk` | 20 x 18 |
| 12 | Route 1 | OVERWORLD | `maps/Route1.blk` | 10 x 18 |
| 13 | Route 2 | OVERWORLD | `maps/Route2.blk` | 10 x 36 |
| 14 | Route 3 | OVERWORLD | `maps/Route3.blk` | 35 x 9 |
| 15 | Route 4 | OVERWORLD | `maps/Route4.blk` | 45 x 9 |
| 16 | Route 5 | OVERWORLD | `maps/Route5.blk` | 10 x 18 |
| 17 | Route 6 | OVERWORLD | `maps/Route6.blk` | 10 x 18 |
| 18 | Route 7 | OVERWORLD | `maps/Route7.blk` | 10 x 9 |
| 19 | Route 8 | OVERWORLD | `maps/Route8.blk` | 30 x 9 |
| 20 | Route 9 | OVERWORLD | `maps/Route9.blk` | 30 x 9 |
| 21 | Route 10 | OVERWORLD | `maps/Route10.blk` | 10 x 36 |
| 22 | Route 11 | OVERWORLD | `maps/Route11.blk` | 30 x 9 |
| 23 | Route 12 | OVERWORLD | `maps/Route12.blk` | 10 x 54 |
| 24 | Route 13 | OVERWORLD | `maps/Route13.blk` | 30 x 9 |
| 25 | Route 14 | OVERWORLD | `maps/Route14.blk` | 10 x 27 |
| 26 | Route 15 | OVERWORLD | `maps/Route15.blk` | 30 x 9 |
| 27 | Route 16 | OVERWORLD | `maps/Route16.blk` | 20 x 9 |
| 28 | Route 17 | OVERWORLD | `maps/Route17.blk` | 10 x 72 |
| 29 | Route 18 | OVERWORLD | `maps/Route18.blk` | 25 x 9 |
| 30 | Route 19 | OVERWORLD | `maps/Route19.blk` | 10 x 27 |
| 31 | Route 20 | OVERWORLD | `maps/Route20.blk` | 50 x 9 |
| 32 | Route 21 | OVERWORLD | `maps/Route21.blk` | 10 x 45 |
| 33 | Route 22 | OVERWORLD | `maps/Route22.blk` | 20 x 9 |
| 34 | Route 23 | PLATEAU | `maps/Route23.blk` | 10 x 72 |
| 35 | Route 24 | OVERWORLD | `maps/Route24.blk` | 10 x 18 |
| 36 | Route 25 | OVERWORLD | `maps/Route25.blk` | 30 x 9 |
| 37 | Reds House 1F | REDS_HOUSE_1 | `maps/RedsHouse1F.blk` | 4 x 4 |
| 38 | Reds House 2F | REDS_HOUSE_2 | `maps/RedsHouse2F.blk` | 4 x 4 |
| 39 | Blues House | HOUSE | `maps/BluesHouse.blk` | 4 x 4 |
| 40 | Oaks Lab | DOJO | `maps/OaksLab.blk` | 5 x 6 |
| 41 | Viridian Pokecenter | POKECENTER | `maps/ViridianPokecenter.blk` | 7 x 4 |
| 42 | Viridian Mart | MART | `maps/ViridianMart.blk` | 4 x 4 |
| 43 | Viridian School | HOUSE | `maps/ViridianSchoolHouse.blk` | 4 x 4 |
| 44 | Viridian House | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 45 | Viridian Gym | GYM | `maps/ViridianGym.blk` | 10 x 9 |
| 46 | Digletts Cave Exit | CAVERN | `maps/DiglettsCaveRoute2.blk` | 4 x 4 |
| 47 | Viridian Forest Exit | FOREST_GATE | `maps/ViridianForestNorthGate.blk` | 5 x 4 |
| 48 | Route 2 House | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 49 | Route 2 Gate | GATE | `maps/ViridianForestNorthGate.blk` | 5 x 4 |
| 50 | Viridian Forest Entrance | FOREST_GATE | `maps/ViridianForestNorthGate.blk` | 5 x 4 |
| 51 | Viridian Forest | FOREST | `maps/ViridianForest.blk` | 17 x 24 |
| 52 | Museum 1F | MUSEUM | `maps/Museum1F.blk` | 10 x 4 |
| 53 | Museum 2F | MUSEUM | `maps/Museum2F.blk` | 7 x 4 |
| 54 | Pewter Gym | GYM | `maps/PewterGym.blk` | 5 x 7 |
| 55 | Pewter House 1 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 56 | Pewter Mart | MART | `maps/PewterMart.blk` | 4 x 4 |
| 57 | Pewter House 2 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 58 | Pewter Pokecenter | POKECENTER | `maps/PewterPokecenter.blk` | 7 x 4 |
| 59 | Mt. Moon 1 | CAVERN | `maps/MtMoon1F.blk` | 20 x 18 |
| 60 | Mt. Moon 2 | CAVERN | `maps/MtMoonB1F.blk` | 14 x 14 |
| 61 | Mt. Moon 3 | CAVERN | `maps/MtMoonB2F.blk` | 20 x 18 |
| 62 | Trashed House | HOUSE | `maps/CeruleanTrashedHouse.blk` | 4 x 4 |
| 63 | Cerulean House 1 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 64 | Cerulean Pokecenter | POKECENTER | `maps/CeruleanPokecenter.blk` | 7 x 4 |
| 65 | Cerulean Gym | GYM | `maps/CeruleanGym.blk` | 5 x 7 |
| 66 | Bike Shop | CLUB | `maps/BikeShop.blk` | 4 x 4 |
| 67 | Cerulean Mart | MART | `maps/VermilionMart.blk` | 4 x 4 |
| 68 | Mt. Moon Pokecenter | POKECENTER | `maps/MtMoonPokecenter.blk` | 7 x 4 |
| 69 | Trashed House Copy | HOUSE | `maps/CeruleanTrashedHouse.blk` | 4 x 4 |
| 70 | Route 5 Gate | GATE | `maps/Route5Gate.blk` | 4 x 3 |
| 71 | Path Entrance Route 5 | GATE | `maps/UndergroundPathRoute5.blk` | 4 x 4 |
| 72 | Daycare | HOUSE | `maps/Daycare.blk` | 4 x 4 |
| 73 | Route 6 Gate | GATE | `maps/Route6Gate.blk` | 4 x 3 |
| 74 | Path Entrance Route 6 | GATE | `maps/UndergroundPathRoute5.blk` | 4 x 4 |
| 75 | Path Entrance Route 6 Copy | GATE | `maps/UndergroundPathRoute5.blk` | 4 x 4 |
| 76 | Route 7 Gate | GATE | `maps/Route7Gate.blk` | 3 x 4 |
| 77 | Path Entrance Route 7 | GATE | `maps/UndergroundPathRoute5.blk` | 4 x 4 |
| 78 | Path Entrance Route 7 Copy | GATE | `maps/UndergroundPathRoute5.blk` | 4 x 4 |
| 79 | Route 8 Gate | GATE | `maps/Route8Gate.blk` | 3 x 4 |
| 80 | Path Entrance Route 8 | GATE | `maps/UndergroundPathRoute8.blk` | 4 x 4 |
| 81 | Rock Tunnel Pokecenter | POKECENTER | `maps/MtMoonPokecenter.blk` | 7 x 4 |
| 82 | Rock Tunnel 1 | CAVERN | `maps/RockTunnel1F.blk` | 20 x 18 |
| 83 | Power Plant | FACILITY | `maps/PowerPlant.blk` | 20 x 18 |
| 84 | Route 11 Gate 1F | GATE | `maps/Route11Gate1F.blk` | 4 x 5 |
| 85 | Digletts Cave Entrance | CAVERN | `maps/DiglettsCaveRoute2.blk` | 4 x 4 |
| 86 | Route 11 Gate 2F | GATE | `maps/Route11Gate2F.blk` | 4 x 4 |
| 87 | Route 12 Gate 1F | GATE | `maps/Route12Gate1F.blk` | 5 x 4 |
| 88 | Bills House | INTERIOR | `maps/BillsHouse.blk` | 4 x 4 |
| 89 | Vermilion Pokecenter | POKECENTER | `maps/PewterPokecenter.blk` | 7 x 4 |
| 90 | Pokemon Fan Club | INTERIOR | `maps/PokemonFanClub.blk` | 4 x 4 |
| 91 | Vermilion Mart | MART | `maps/VermilionMart.blk` | 4 x 4 |
| 92 | Vermilion Gym | GYM | `maps/VermilionGym.blk` | 5 x 9 |
| 93 | Vermilion House 1 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 94 | Vermilion Dock | SHIP_PORT | `maps/VermilionDock.blk` | 14 x 6 |
| 95 | S.S. Anne 1 | SHIP | `maps/SSAnne1F.blk` | 20 x 9 |
| 96 | S.S. Anne 2 | SHIP | `maps/SSAnne2F.blk` | 20 x 9 |
| 97 | S.S. Anne 3 | SHIP | `maps/SSAnne3F.blk` | 10 x 3 |
| 98 | S.S. Anne 4 | SHIP | `maps/SSAnneB1F.blk` | 15 x 4 |
| 99 | S.S. Anne 5 | SHIP | `maps/SSAnneBow.blk` | 10 x 7 |
| 100 | S.S. Anne 6 | SHIP | `maps/SSAnneKitchen.blk` | 7 x 8 |
| 101 | S.S. Anne 7 | SHIP | `maps/SSAnneCaptainsRoom.blk` | 3 x 4 |
| 102 | S.S. Anne 8 | SHIP | `maps/SSAnne1FRooms.blk` | 12 x 8 |
| 103 | S.S. Anne 9 | SHIP | `maps/SSAnne2FRooms.blk` | 12 x 8 |
| 104 | S.S. Anne 10 | SHIP | `maps/SSAnne2FRooms.blk` | 12 x 8 |
| 108 | Victory Road 1 | CAVERN | `maps/VictoryRoad1F.blk` | 10 x 9 |
| 113 | Lances Room | DOJO | `maps/LancesRoom.blk` | 13 x 13 |
| 118 | Hall of Fame | GYM | `maps/HallOfFame.blk` | 5 x 4 |
| 119 | Underground Path NS | UNDERGROUND | `maps/UndergroundPathNorthSouth.blk` | 4 x 24 |
| 120 | Champions Room | GYM | `maps/ChampionsRoom.blk` | 4 x 4 |
| 121 | Underground Path WE | UNDERGROUND | `maps/UndergroundPathWestEast.blk` | 25 x 4 |
| 122 | Celadon Mart 1 | LOBBY | `maps/CeladonMart1F.blk` | 10 x 4 |
| 123 | Celadon Mart 2 | LOBBY | `maps/CeladonMart2F.blk` | 10 x 4 |
| 124 | Celadon Mart 3 | LOBBY | `maps/CeladonMart3F.blk` | 10 x 4 |
| 125 | Celadon Mart 4 | LOBBY | `maps/CeladonMart4F.blk` | 10 x 4 |
| 126 | Celadon Mart Roof | LOBBY | `maps/CeladonMartRoof.blk` | 10 x 4 |
| 127 | Celadon Mart Elevator | LOBBY | `maps/CeladonMartElevator.blk` | 2 x 2 |
| 128 | Celadon Mansion 1 | MANSION | `maps/CeladonMansion1F.blk` | 4 x 6 |
| 129 | Celadon Mansion 2 | MANSION | `maps/CeladonMansion2F.blk` | 4 x 6 |
| 130 | Celadon Mansion 3 | MANSION | `maps/CeladonMansion3F.blk` | 4 x 6 |
| 131 | Celadon Mansion 4 | MANSION | `maps/CeladonMansionRoof.blk` | 4 x 6 |
| 132 | Celadon Mansion 5 | HOUSE | `maps/ViridianSchoolHouse.blk` | 4 x 4 |
| 133 | Celadon Pokecenter | POKECENTER | `maps/MtMoonPokecenter.blk` | 7 x 4 |
| 134 | Celadon Gym | GYM | `maps/CeladonGym.blk` | 5 x 9 |
| 135 | Game Corner | LOBBY | `maps/GameCorner.blk` | 10 x 9 |
| 136 | Celadon Mart 5 | LOBBY | `maps/CeladonMart5F.blk` | 10 x 4 |
| 137 | Celadon Prize Room | LOBBY | `maps/GameCornerPrizeRoom.blk` | 5 x 4 |
| 138 | Celadon Diner | LOBBY | `maps/CeladonDiner.blk` | 5 x 4 |
| 139 | Celadon House | MANSION | `maps/CeladonChiefHouse.blk` | 4 x 4 |
| 140 | Celadon Hotel | POKECENTER | `maps/CeladonHotel.blk` | 7 x 4 |
| 141 | Lavender Pokecenter | POKECENTER | `maps/PewterPokecenter.blk` | 7 x 4 |
| 142 | Pokemon Tower 1 | CEMETERY | `maps/PokemonTower1F.blk` | 10 x 9 |
| 143 | Pokemon Tower 2 | CEMETERY | `maps/PokemonTower2F.blk` | 10 x 9 |
| 144 | Pokemon Tower 3 | CEMETERY | `maps/PokemonTower3F.blk` | 10 x 9 |
| 145 | Pokemon Tower 4 | CEMETERY | `maps/PokemonTower4F.blk` | 10 x 9 |
| 146 | Pokemon Tower 5 | CEMETERY | `maps/PokemonTower5F.blk` | 10 x 9 |
| 147 | Pokemon Tower 6 | CEMETERY | `maps/PokemonTower6F.blk` | 10 x 9 |
| 148 | Pokemon Tower 7 | CEMETERY | `maps/PokemonTower7F.blk` | 10 x 9 |
| 149 | Lavender House 1 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 150 | Lavender Mart | MART | `maps/VermilionMart.blk` | 4 x 4 |
| 151 | Lavender House 2 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 152 | Fuchsia Mart | MART | `maps/FuchsiaMart.blk` | 4 x 4 |
| 153 | Fuchsia House 1 | HOUSE | `maps/FuchsiaBillsGrandpasHouse.blk` | 4 x 4 |
| 154 | Fuchsia Pokecenter | POKECENTER | `maps/FuchsiaPokecenter.blk` | 7 x 4 |
| 155 | Fuchsia House 2 | LAB | `maps/WardensHouse.blk` | 5 x 4 |
| 156 | Safari Zone Entrance | GATE | `maps/SafariZoneGate.blk` | 4 x 3 |
| 157 | Fuchsia Gym | GYM | `maps/FuchsiaGym.blk` | 5 x 9 |
| 158 | Fuchsia Meeting Room | LAB | `maps/FuchsiaMeetingRoom.blk` | 7 x 4 |
| 159 | Seafoam Islands 2 | CAVERN | `maps/SeafoamIslandsB1F.blk` | 15 x 9 |
| 160 | Seafoam Islands 3 | CAVERN | `maps/SeafoamIslandsB2F.blk` | 15 x 9 |
| 161 | Seafoam Islands 4 | CAVERN | `maps/SeafoamIslandsB3F.blk` | 15 x 9 |
| 162 | Seafoam Islands 5 | CAVERN | `maps/SeafoamIslandsB4F.blk` | 15 x 9 |
| 163 | Vermilion House 2 | HOUSE | `maps/Daycare.blk` | 4 x 4 |
| 164 | Fuchsia House 3 | SHIP | `maps/FuchsiaGoodRodHouse.blk` | 4 x 4 |
| 165 | Mansion 1 | FACILITY | `maps/PokemonMansion1F.blk` | 15 x 14 |
| 166 | Cinnabar Gym | FACILITY | `maps/CinnabarGym.blk` | 10 x 9 |
| 167 | Cinnabar Lab 1 | LAB | `maps/CinnabarLab.blk` | 9 x 4 |
| 168 | Cinnabar Lab 2 | LAB | `maps/CinnabarLabTradeRoom.blk` | 4 x 4 |
| 169 | Cinnabar Lab 3 | LAB | `maps/CinnabarLabMetronomeRoom.blk` | 4 x 4 |
| 170 | Cinnabar Lab 4 | LAB | `maps/CinnabarLabFossilRoom.blk` | 4 x 4 |
| 171 | Cinnabar Pokecenter | POKECENTER | `maps/FuchsiaPokecenter.blk` | 7 x 4 |
| 172 | Cinnabar Mart | MART | `maps/PewterMart.blk` | 4 x 4 |
| 173 | Cinnabar Mart Copy | MART | `maps/PewterMart.blk` | 4 x 4 |
| 174 | Indigo Plateau Lobby | MART | `maps/IndigoPlateauLobby.blk` | 8 x 6 |
| 175 | Copycats House 1F | REDS_HOUSE_1 | `maps/CopycatsHouse1F.blk` | 4 x 4 |
| 176 | Copycats House 2F | REDS_HOUSE_2 | `maps/RedsHouse2F.blk` | 4 x 4 |
| 177 | Fighting Dojo | DOJO | `maps/FightingDojo.blk` | 5 x 6 |
| 178 | Saffron Gym | FACILITY | `maps/SaffronGym.blk` | 10 x 9 |
| 179 | Saffron House 1 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 180 | Saffron Mart | MART | `maps/VermilionMart.blk` | 4 x 4 |
| 181 | Silph Co 1F | FACILITY | `maps/SilphCo1F.blk` | 15 x 9 |
| 182 | Saffron Pokecenter | POKECENTER | `maps/PewterPokecenter.blk` | 7 x 4 |
| 183 | Saffron House 2 | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 184 | Route 15 Gate 1F | GATE | `maps/Route11Gate1F.blk` | 4 x 5 |
| 185 | Route 15 Gate 2F | GATE | `maps/Route11Gate2F.blk` | 4 x 4 |
| 186 | Route 16 Gate 1F | GATE | `maps/Route16Gate1F.blk` | 4 x 7 |
| 187 | Route 16 Gate 2F | GATE | `maps/Route11Gate2F.blk` | 4 x 4 |
| 188 | Route 16 House | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 189 | Route 12 House | HOUSE | `maps/Daycare.blk` | 4 x 4 |
| 190 | Route 18 Gate 1F | GATE | `maps/Route11Gate1F.blk` | 4 x 5 |
| 191 | Route 18 Gate 2F | GATE | `maps/Route11Gate2F.blk` | 4 x 4 |
| 192 | Seafoam Islands 1 | CAVERN | `maps/SeafoamIslands1F.blk` | 15 x 9 |
| 193 | Route 22 Gate | GATE | `maps/Route22Gate.blk` | 5 x 4 |
| 194 | Victory Road 2 | CAVERN | `maps/VictoryRoad2F.blk` | 15 x 9 |
| 195 | Route 12 Gate 2F | GATE | `maps/Route11Gate2F.blk` | 4 x 4 |
| 196 | Vermilion House 3 | HOUSE | `maps/VermilionTradeHouse.blk` | 4 x 4 |
| 197 | Digletts Cave | CAVERN | `maps/DiglettsCave.blk` | 20 x 18 |
| 198 | Victory Road 3 | CAVERN | `maps/VictoryRoad3F.blk` | 15 x 9 |
| 199 | Rocket Hideout 1 | FACILITY | `maps/RocketHideoutB1F.blk` | 15 x 14 |
| 200 | Rocket Hideout 2 | FACILITY | `maps/RocketHideoutB2F.blk` | 15 x 14 |
| 201 | Rocket Hideout 3 | FACILITY | `maps/RocketHideoutB3F.blk` | 15 x 14 |
| 202 | Rocket Hideout 4 | FACILITY | `maps/RocketHideoutB4F.blk` | 15 x 12 |
| 203 | Rocket Hideout Elevator | LOBBY | `maps/RocketHideoutElevator.blk` | 3 x 4 |
| 207 | Silph Co 2F | FACILITY | `maps/SilphCo2F.blk` | 15 x 9 |
| 208 | Silph Co 3F | FACILITY | `maps/SilphCo3F.blk` | 15 x 9 |
| 209 | Silph Co 4F | FACILITY | `maps/SilphCo4F.blk` | 15 x 9 |
| 210 | Silph Co 5F | FACILITY | `maps/SilphCo5F.blk` | 15 x 9 |
| 211 | Silph Co 6F | FACILITY | `maps/SilphCo6F.blk` | 13 x 9 |
| 212 | Silph Co 7F | FACILITY | `maps/SilphCo7F.blk` | 13 x 9 |
| 213 | Silph Co 8F | FACILITY | `maps/SilphCo8F.blk` | 13 x 9 |
| 214 | Mansion 2 | FACILITY | `maps/PokemonMansion2F.blk` | 15 x 14 |
| 215 | Mansion 3 | FACILITY | `maps/PokemonMansion3F.blk` | 15 x 9 |
| 216 | Mansion 4 | FACILITY | `maps/PokemonMansionB1F.blk` | 15 x 14 |
| 217 | Safari Zone East | FOREST | `maps/SafariZoneEast.blk` | 15 x 13 |
| 218 | Safari Zone North | FOREST | `maps/SafariZoneNorth.blk` | 20 x 18 |
| 219 | Safari Zone West | FOREST | `maps/SafariZoneWest.blk` | 15 x 13 |
| 220 | Safari Zone Center | FOREST | `maps/SafariZoneCenter.blk` | 15 x 13 |
| 221 | Safari Zone Rest House 1 | GATE | `maps/SafariZoneCenterRestHouse.blk` | 4 x 4 |
| 222 | Safari Zone Secret House | LAB | `maps/SafariZoneSecretHouse.blk` | 4 x 4 |
| 223 | Safari Zone Rest House 2 | GATE | `maps/SafariZoneCenterRestHouse.blk` | 4 x 4 |
| 224 | Safari Zone Rest House 3 | GATE | `maps/SafariZoneCenterRestHouse.blk` | 4 x 4 |
| 225 | Safari Zone Rest House 4 | GATE | `maps/SafariZoneCenterRestHouse.blk` | 4 x 4 |
| 226 | Unknown Dungeon 2 | CAVERN | `maps/CeruleanCave2F.blk` | 15 x 9 |
| 227 | Unknown Dungeon 3 | CAVERN | `maps/CeruleanCaveB1F.blk` | 15 x 9 |
| 228 | Unknown Dungeon 1 | CAVERN | `maps/CeruleanCave1F.blk` | 15 x 9 |
| 229 | Name Raters House | HOUSE | `maps/ViridianNicknameHouse.blk` | 4 x 4 |
| 230 | Cerulean House 2 | SHIP | `maps/CeruleanBadgeHouse.blk` | 4 x 4 |
| 232 | Rock Tunnel 2 | CAVERN | `maps/RockTunnelB1F.blk` | 20 x 18 |
| 233 | Silph Co 9F | FACILITY | `maps/SilphCo9F.blk` | 13 x 9 |
| 234 | Silph Co 10F | FACILITY | `maps/SilphCo10F.blk` | 8 x 9 |
| 235 | Silph Co 11F | INTERIOR | `maps/SilphCo11F.blk` | 9 x 9 |
| 236 | Silph Co Elevator | LOBBY | `maps/SilphCoElevator.blk` | 2 x 2 |
| 239 | Trade Center | CLUB | `maps/TradeCenter.blk` | 5 x 4 |
| 240 | Colosseum | CLUB | `maps/Colosseum.blk` | 5 x 4 |
| 245 | Loreleis Room | GYM | `maps/LoreleisRoom.blk` | 5 x 6 |
| 246 | Brunos Room | GYM | `maps/BrunosRoom.blk` | 5 x 6 |
| 247 | Agathas Room | CEMETERY | `maps/AgathasRoom.blk` | 5 x 6 |
