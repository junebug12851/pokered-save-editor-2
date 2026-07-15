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

/**
 * WILD POKÉMON — the map's grass and water encounter tables. A panel in the left dock.
 *
 * Two identical sections (grass, then water — surfing & fishing). Each has an **Enable** switch, an
 * **encounter-chance** slider (Low↔High), and the ten encounter slots drawn like the Pokémon box:
 * the slot's fixed **percent** upper-left, an editable **level** upper-right, the mon's artwork in
 * the middle. Click a slot to pick its species (the same list as the Pokémon details screen); drag a
 * slot to reorder — which re-weights the odds, because a slot's chance is its position (pokered's
 * WildMonEncounterSlotChances). Enabling a blank table seeds ten random mons.
 *
 * ⚠️ On disk each slot is (LEVEL, SPECIES) and the species is the internal index, not the Pokédex
 * number — the model reads/writes it correctly; this panel only ever names slots and values.
 * See notes/reference/wild-encounters.md.
 *
 * @see notes/plans/map-screen.md → Phase 8 (Encounters)
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: panel
  objectName: "mapWildPokemon"

  /// The map canvas — handed over by MapDock's Loader. Unused here (this panel edits the map itself,
  /// not a selection), but kept for the dock's uniform panel contract.
  property var canvas: null

  /// Re-ask the model whenever anything changes (an edit, an enable, a reorder). Same pattern as the
  /// Details panel — a plain binding on grassMons() would not know the values moved under it.
  property int revision: 0
  Connections {
    target: brg.map
    function onChanged() { panel.revision++; }
  }

  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
      width: scroller.availableWidth - 16
      spacing: 14

      Label {
        Layout.fillWidth: true
        Layout.topMargin: 10
        Layout.leftMargin: 4
        text: qsTr("Wild Pokémon")
        font.bold: true
        font.pixelSize: 15
        color: brg.settings.textColorDark
      }

      Label {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        text: qsTr("The Pokémon that appear in the grass and water on this map. Rarest is last — a "
                   + "slot's chance is its place in the list, so drag to reorder.")
        wrapMode: Text.Wrap
        opacity: 0.6
        font.pixelSize: 11
        color: brg.settings.textColorDark
      }

      WildMonList {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        kind: "grass"
        title: qsTr("Grass")
        subtitle: qsTr("Walking through tall grass")
        revision: panel.revision
      }

      Rectangle { Layout.fillWidth: true; Layout.leftMargin: 4; Layout.rightMargin: 4
                  height: 1; color: Qt.rgba(0, 0, 0, 0.10) }

      WildMonList {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.bottomMargin: 12
        kind: "water"
        title: qsTr("Water")
        subtitle: qsTr("Surfing and fishing")
        revision: panel.revision
      }
    }
  }
}
