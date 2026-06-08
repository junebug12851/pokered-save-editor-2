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

    function test_modelsAreReachable() {
        // The item-model objects the views bind to must be live objects from QML.
        verify(brg.typesModel !== null, "typesModel null")
        verify(brg.pokedexModel !== null, "pokedexModel null")
        verify(brg.marketModel !== null, "marketModel null")
    }
}
