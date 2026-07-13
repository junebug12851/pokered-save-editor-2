/*
  * Copyright 2026 Twilight
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

import QtQuick
import QtTest

// Drives the C++<->QML data pipeline from QML's perspective, the way the real
// screens do via `brg.*`. The point is the property CHAIN: every hop must resolve
// to a real object (never `undefined`) and leaf values must read AND write back.
TestCase {
    name: "BrgChain"

    function test_bridge_isExposed() {
        verify(typeof brg !== "undefined" && brg !== null, "brg context property missing")
        verify(brg.file !== null, "brg.file null")
        verify(brg.file.data !== null, "brg.file.data null")
        verify(brg.file.data.dataExpanded !== null, "brg.file.data.dataExpanded null")
    }

    function test_playerBasicsChain_readsAndWrites() {
        var basics = brg.file.data.dataExpanded.player.basics
        verify(basics !== null && basics !== undefined, "player.basics did not resolve")

        // Leaf values must be real numbers, not undefined (the regression).
        verify(typeof basics.money === "number", "money is not a number (undefined chain?)")
        verify(typeof basics.coins === "number", "coins is not a number")

        basics.money = 123456
        compare(basics.money, 123456, "money did not write back through the chain")
        basics.coins = 4321
        compare(basics.coins, 4321, "coins did not write back through the chain")
    }

    function test_deepWorldChain_resolves() {
        // A deeper hop: brg.file.data.dataExpanded.world.other.playtime.hours
        var pt = brg.file.data.dataExpanded.world.other.playtime
        verify(pt !== null && pt !== undefined, "world.other.playtime did not resolve")
        verify(typeof pt.hours === "number", "playtime.hours is not a number")
        pt.hours = 99
        compare(pt.hours, 99, "playtime.hours did not write back")
    }

    // ── The AREA children QML traverses ──────────────────────────────────────────────────────
    //
    // This is the test that SHOULD have existed on 2026-07-12 and didn't. The Map screen's new
    // Music panel read "No save open" with a save wide open, because `AreaAudio` was still
    // Q_DECLARE_OPAQUE_POINTER'd in area.h -- which makes QML read the entire chain past it as
    // `undefined`, silently. Nothing warned. Every binding "worked". The whole feature was dead.
    //
    // As of 2026-07-12 (map-screen Phase 0) NONE of the eleven is opaque: the new Map screen edits
    // the whole Area block from QML. This test walks ALL of them, so re-opaquing any one of them --
    // for build speed, or by accident -- fails here by name instead of shipping a screen that
    // quietly does nothing.
    function test_areaChildrenQmlTraverses_resolve() {
        var area = brg.file.data.dataExpanded.area
        verify(area !== null && area !== undefined, "area did not resolve")

        var children = ["general", "audio", "map", "player", "tileset", "warps", "signs",
                        "sprites", "pokemon", "npc", "preloadedSprites"]

        for (var i = 0; i < children.length; i++) {
            var c = children[i]
            verify(area[c] !== null && area[c] !== undefined,
                   "area." + c + " did not resolve -- is it Q_DECLARE_OPAQUE_POINTER'd again?")
        }
    }

    // Every Area child, read AND written from QML. A resolving object with `undefined` leaves is the
    // same bug wearing a hat, so the leaves are what this checks.
    function test_areaLeaves_readAndWriteThroughTheChain() {
        var area = brg.file.data.dataExpanded.area

        // AreaMap -- the map's identity, size, pointers and scratch
        verify(typeof area.map.curMap === "number", "area.map.curMap is not a number")
        area.map.curMap = 12
        compare(area.map.curMap, 12, "area.map.curMap did not write back")
        area.map.outOfBoundsBlock = 0x0B
        compare(area.map.outOfBoundsBlock, 0x0B, "area.map.outOfBoundsBlock did not write back")

        // AreaPlayer -- where he stands and which way he faces
        verify(typeof area.player.xCoord === "number", "area.player.xCoord is not a number")
        area.player.xCoord = 5
        area.player.yCoord = 6
        compare(area.player.xCoord, 5, "area.player.xCoord did not write back")
        compare(area.player.yCoord, 6, "area.player.yCoord did not write back")
        verify(typeof area.player.surfingAllowed === "boolean",
               "area.player.surfingAllowed is not a boolean")

        // AreaTileset -- incl. the animation byte (0x3522) the whole map animation hangs off
        verify(typeof area.tileset.type === "number", "area.tileset.type is not a number")
        area.tileset.grassTile = 0x52
        compare(area.tileset.grassTile, 0x52, "area.tileset.grassTile did not write back")

        // AreaPokemon -- the wild encounter tables (no UI before the map overhaul)
        verify(typeof area.pokemon.grassRate === "number", "area.pokemon.grassRate is not a number")
        area.pokemon.grassRate = 25
        compare(area.pokemon.grassRate, 25, "area.pokemon.grassRate did not write back")

        // AreaNPC / AreaWarps state flags
        verify(typeof area.npc.npcsFaceAway === "boolean", "area.npc.npcsFaceAway is not a boolean")
        verify(typeof area.warps.warpDest === "number", "area.warps.warpDest is not a number")

        // AreaLoadedSprites
        verify(typeof area.preloadedSprites.loadedSetId === "number",
               "area.preloadedSprites.loadedSetId is not a number")
    }

    // The OBJECTS the map screen drags around come back from Q_INVOKABLEs, not properties. They are
    // parentless QObjects, so they are qmlCppOwned()-protected in C++ -- and QML has to be able to
    // read their fields, or the whole object layer is dead.
    function test_areaObjectFragments_areReachableFromQml() {
        var area = brg.file.data.dataExpanded.area

        verify(typeof area.warps.warpCount() === "number", "warpCount() is not callable")
        verify(typeof area.signs.signCount() === "number", "signCount() is not callable")
        verify(typeof area.sprites.spriteCount() === "number", "spriteCount() is not callable")

        // Make sure there is at least one of each to look at, then read + write its coords.
        if (area.warps.warpCount() === 0)
            area.warps.warpNew()
        var warp = area.warps.warpAt(0)
        verify(warp !== null && warp !== undefined, "warps.warpAt(0) did not resolve")
        warp.x = 4; warp.y = 7
        compare(warp.x, 4, "warp.x did not write back")
        compare(warp.y, 7, "warp.y did not write back")
        verify(typeof warp.destMap === "number", "warp.destMap is not a number")

        if (area.signs.signCount() === 0)
            area.signs.signNew()
        var sign = area.signs.signAt(0)
        verify(sign !== null && sign !== undefined, "signs.signAt(0) did not resolve")
        sign.x = 3; sign.y = 2
        compare(sign.x, 3, "sign.x did not write back")
        verify(typeof sign.txtId === "number", "sign.txtId is not a number")

        if (area.sprites.spriteCount() === 0)
            area.sprites.spriteNew()
        var npc = area.sprites.spriteAt(0)
        verify(npc !== null && npc !== undefined, "sprites.spriteAt(0) did not resolve")
        npc.mapX = 8; npc.mapY = 9
        compare(npc.mapX, 8, "sprite.mapX did not write back")
        verify(typeof npc.pictureID === "number", "sprite.pictureID is not a number")
        verify(typeof npc.movementByte === "number", "sprite.movementByte is not a number")
    }

    function test_areaAudioChain_readsAndWrites() {
        var audio = brg.file.data.dataExpanded.area.audio

        // The four bytes/bits the Music panel edits. `undefined` here is the exact failure mode.
        verify(typeof audio.musicID === "number", "audio.musicID is not a number (undefined chain?)")
        verify(typeof audio.musicBank === "number", "audio.musicBank is not a number")
        verify(typeof audio.noAudioFadeout === "boolean", "audio.noAudioFadeout is not a boolean")
        verify(typeof audio.preventMusicChange === "boolean",
               "audio.preventMusicChange is not a boolean")

        audio.musicID = 187          // an inner voice: Pallet Town's channel 2, alone
        audio.musicBank = 2
        compare(audio.musicID, 187, "musicID did not write back through the chain")
        compare(audio.musicBank, 2, "musicBank did not write back through the chain")

        audio.noAudioFadeout = true
        compare(audio.noAudioFadeout, true, "noAudioFadeout did not write back")
        audio.preventMusicChange = true
        compare(audio.preventMusicChange, true, "preventMusicChange did not write back")
    }

    function test_musicPlayerIsExposed() {
        verify(brg.music !== null && brg.music !== undefined, "brg.music did not resolve")
        verify(brg.music.dataReady === true, "the music data did not load")

        // Every piece of music: the 46 real tracks AND their 105 inner voices.
        var t = brg.music.tracks
        verify(t.length > 140, "the track list is too short -- are the inner voices missing?")

        var real = 0, inner = 0
        for (var i = 0; i < t.length; i++)
            t[i].inner ? inner++ : real++
        compare(real, 46, "there should be 46 real tracks")
        compare(inner, 105, "there should be 105 inner voices")
    }

    function test_modelsAreReachable() {
        // The item-model objects the views bind to must be live objects from QML.
        verify(brg.typesModel !== null, "typesModel null")
        verify(brg.pokedexModel !== null, "pokedexModel null")
        verify(brg.marketModel !== null, "marketModel null")
    }
}
