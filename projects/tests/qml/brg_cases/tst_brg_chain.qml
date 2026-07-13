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
    // `area.h` deliberately keeps most children opaque (build time) and full-includes only the ones
    // QML walks. So: WHENEVER QML STARTS TRAVERSING A NEW AREA CHILD, ADD IT HERE. If the include is
    // ever dropped, this fails immediately instead of shipping a screen that quietly does nothing.
    function test_areaChildrenQmlTraverses_resolve() {
        var area = brg.file.data.dataExpanded.area
        verify(area !== null && area !== undefined, "area did not resolve")

        // general -- the playtime helper (has been traversed for a long time)
        verify(area.general !== null && area.general !== undefined,
               "area.general did not resolve (is AreaGeneral opaque again?)")

        // audio -- the Map screen's Music panel
        verify(area.audio !== null && area.audio !== undefined,
               "area.audio did not resolve -- Q_DECLARE_OPAQUE_POINTER(AreaAudio*) is back")
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
