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
 * One wild-encounter table (grass OR water), drawn like the Pokémon box.
 *
 * A titled section with an **Enable** switch (rate > 0), an **encounter-chance** slider (Low↔High),
 * and the ten slots as box-style cells: the slot's **percent** (its fixed rarity — pokered's own
 * WildMonEncounterSlotChances) upper-left, the **level** (directly editable) upper-right, the mon's
 * artwork in the middle, its name below. Clicking a cell opens the **species picker** (the same
 * SelectSpecies list as the Pokémon details screen); dragging a cell **reorders** the list, which
 * re-weights rarity because a slot's chance is its position. You cannot leave a slot out — the game
 * reads all ten when the rate is > 0. See notes/reference/wild-encounters.md.
 *
 * `kind` selects which table this edits ("grass" or "water"); everything routes through brg.map so
 * an edit writes only the byte(s) it names.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../../../fragments/controls/selection"

ColumnLayout {
  id: sect

  property string kind: "grass"       // "grass" | "water"
  property string title: qsTr("Grass")
  property string subtitle: ""
  property int revision: 0            // bumped by the panel on brg.map.changed

  readonly property bool isGrass: kind === "grass"
  readonly property bool enabled: { sect.revision; return isGrass ? brg.map.grassEnabled : brg.map.waterEnabled }
  readonly property int  rate:    { sect.revision; return isGrass ? brg.map.grassRate    : brg.map.waterRate }
  readonly property var  mons:    { sect.revision; return isGrass ? brg.map.grassMons()  : brg.map.waterMons() }

  function setEnabled(on)      { if (isGrass) brg.map.grassEnabled = on;  else brg.map.waterEnabled = on }
  function setRate(v)          { if (isGrass) brg.map.grassRate = v;      else brg.map.waterRate = v }
  function setSpecies(slot, i) { if (isGrass) brg.map.setGrassMonSpecies(slot, i); else brg.map.setWaterMonSpecies(slot, i) }
  function setLevel(slot, l)   { if (isGrass) brg.map.setGrassMonLevel(slot, l);   else brg.map.setWaterMonLevel(slot, l) }
  function moveMon(from, to)   { if (isGrass) brg.map.moveGrassMon(from, to);      else brg.map.moveWaterMon(from, to) }

  // The mon-icon path, matching PokemonBoxView: mon-icons/<dex 3>-<name>.svg. The DB's dex is
  // 0-based (Bulbasaur = 0), and the files are 1-based (001-bulbasaur), so +1 — exactly the box's
  // (itemDex+1). dex −1 (a glitch species with no Pokédex number) → "?".
  function iconUrl(d) {
    if (d.dex === undefined || d.dex < 0)
      return "qrc:/assets/icons/fontawesome/question.svg";
    var num = (d.dex + 1).toString().padStart(3, "0");
    var name = (d.name || "").toLowerCase();
    if (name === "nidoran<f>")      name = "nidoran-f";
    else if (name === "nidoran<m>") name = "nidoran-m";
    else if (name === "mr.mime")    name = "mrmime";
    return "qrc:/assets/icons/mon-icons/" + num + "-" + name + ".svg";
  }

  function prettyName(n) {
    if (!n) return "";
    n = n.replace("<f>", " ♀").replace("<F>", " ♀");
    n = n.replace("<m>", " ♂").replace("<M>", " ♂");
    n = n.replace("Mr.Mime", "Mr. Mime");
    return n;
  }

  spacing: 6
  Layout.fillWidth: true

  // ── Header: title + enable switch ──────────────────────────────────────────────
  RowLayout {
    Layout.fillWidth: true
    spacing: 8

    ColumnLayout {
      Layout.fillWidth: true
      spacing: 0
      Label {
        text: sect.title
        font.bold: true
        font.pixelSize: 14
        color: brg.settings.textColorDark
      }
      Label {
        visible: sect.subtitle !== ""
        text: sect.subtitle
        font.pixelSize: 10
        opacity: 0.6
        color: brg.settings.textColorDark
      }
    }

    MapSwitch {
      objectName: "wildEnable_" + sect.kind
      checked: sect.enabled
      onToggled: sect.setEnabled(!sect.enabled)
    }
  }

  // Nothing here when there are no wild Pokémon of this kind — said plainly, not an empty grid.
  Label {
    visible: !sect.enabled
    Layout.fillWidth: true
    Layout.bottomMargin: 4
    text: sect.isGrass
          ? qsTr("No wild Pokémon in the grass here. Turn it on to add the ten encounters.")
          : qsTr("No wild Pokémon in the water here. Turn it on to add the ten encounters.")
    wrapMode: Text.Wrap
    opacity: 0.6
    font.pixelSize: 11
    color: brg.settings.textColorDark
  }

  // ── Encounter-chance slider (Low ── High) ──────────────────────────────────────
  ColumnLayout {
    visible: sect.enabled
    Layout.fillWidth: true
    spacing: 2

    Label {
      text: qsTr("Encounter chance")
      font.pixelSize: 11
      color: brg.settings.textColorMid
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 6

      Label { text: qsTr("Low");  font.pixelSize: 10; color: brg.settings.textColorMid }

      Slider {
        id: rateSlider
        Layout.fillWidth: true
        Layout.preferredHeight: 24
        from: 1; to: 255; stepSize: 1
        // Bind to the model so an external change (Enable seeding 25) is reflected; write on move.
        value: Math.max(1, sect.rate)
        onMoved: sect.setRate(Math.round(value))

        MapToolTip {
          parent: rateSlider.handle
          shown: rateSlider.pressed || rateSlider.hovered
          followGlobalSetting: false
          delay: 0
          text: Math.round(rateSlider.value)
        }
      }

      Label { text: qsTr("High"); font.pixelSize: 10; color: brg.settings.textColorMid }
    }
  }

  // ── The ten slots, drawn like the box ──────────────────────────────────────────
  GridLayout {
    id: grid
    visible: sect.enabled
    Layout.fillWidth: true
    columns: 2
    columnSpacing: 6
    rowSpacing: 6

    Repeater {
      model: sect.mons

      // Each cell is a DropArea (a drop reorders); its inner `content` is the draggable card.
      DropArea {
        id: cell
        required property var modelData
        required property int index

        Layout.fillWidth: true
        Layout.preferredHeight: 96

        readonly property var d: modelData
        property int slotIndex: index

        onDropped: (drop) => {
          var s = drop.source;
          if (s === null || s === undefined) return;
          var from = s.slotIndex;
          var to = cell.slotIndex;
          // Defer: the move resets the list (rebuilding delegates) and we want the ghost home first.
          Qt.callLater(function() { sect.moveMon(from, to); });
        }

        // Caret on the cell's left edge while a drag hovers it.
        Rectangle {
          width: 3
          height: parent.height - 12
          anchors.left: parent.left
          anchors.leftMargin: -2
          anchors.verticalCenter: parent.verticalCenter
          radius: 1.5
          z: 50
          visible: cell.containsDrag
          color: brg.settings.accentColor
        }

        Item {
          id: content
          width: cell.width
          height: cell.height
          anchors.horizontalCenter: parent.horizontalCenter
          anchors.verticalCenter: parent.verticalCenter

          // Info the DropArea reads off the dragged item.
          property int slotIndex: cell.slotIndex

          Drag.active: dragArea.drag.active
          Drag.source: content
          Drag.hotSpot.x: width / 2
          Drag.hotSpot.y: height / 2

          Rectangle {
            id: card
            anchors.fill: parent
            radius: 8
            color: dragArea.containsMouse ? Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(0, 0, 0, 0.03)
            border.width: 1
            border.color: Qt.rgba(0, 0, 0, 0.10)
          }

          // The hidden species picker: clicking the card opens its popup (the same species list as
          // the Pokémon details screen). It renders nothing itself — the visuals sit on top.
          SelectSpecies {
            id: picker
            anchors.fill: parent
            z: 0
            currentIndex: picker.indexOfValue(cell.d.index)
            onActivated: sect.setSpecies(cell.slotIndex, picker.currentValue)
            indicator: null
            background: Item {}
            contentItem: Item {}
          }

          // Artwork — mouse-transparent, so a click falls through to the drag/click handler.
          Image {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 4
            width: 52; height: 52
            sourceSize.width: width
            sourceSize.height: height
            source: sect.iconUrl(cell.d)
            fillMode: Image.PreserveAspectFit
            z: 1
          }

          // Percent, upper-left (read-only — it is the slot's position).
          Label {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 5
            z: 2
            text: (cell.d.percent !== undefined ? cell.d.percent.toFixed(1) : "0.0") + "%"
            font.pixelSize: 11
            font.bold: true
            color: brg.settings.textColorMid
          }

          // Name, bottom.
          Label {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 5
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            z: 2
            horizontalAlignment: Text.AlignHCenter
            text: sect.prettyName(cell.d.name)
            font.pixelSize: 11
            font.capitalization: Font.Capitalize
            color: cell.d.glitch ? brg.settings.primaryColor : brg.settings.textColorDark
            elide: Text.ElideRight
          }

          // Level, upper-right — directly editable. A plain TextInput (not a Material TextField,
          // whose tall implicit height gets clipped in this 20px pill). Above the click/drag handler
          // (z:3) so it takes its own clicks; editing it never opens the species picker.
          Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 4
            z: 3
            width: lvlRow.width + 12
            height: 20
            radius: 10
            color: brg.settings.accentColor

            Row {
              id: lvlRow
              anchors.centerIn: parent
              spacing: 2

              Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "L"
                font.pixelSize: 11
                color: brg.settings.textColorLight
              }
              TextInput {
                id: lvlField
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(14, contentWidth)
                text: String(cell.d.level)
                color: brg.settings.textColorLight
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 0; top: 255 }
                selectByMouse: true
                onEditingFinished: sect.setLevel(cell.slotIndex, parseInt(lvlField.text || "0"))
              }
            }
          }

          // Click opens the species picker; a drag past the threshold reorders. Below the level chip.
          MouseArea {
            id: dragArea
            anchors.fill: parent
            z: 2
            hoverEnabled: true
            preventStealing: true
            cursorShape: Qt.PointingHandCursor
            drag.target: content
            drag.threshold: 8
            onClicked: picker.popup.open()
          }

          // While dragging: float in the overlay so the ghost isn't clipped; fade to read as lifted.
          states: State {
            when: content.Drag.active
            ParentChange { target: content; parent: Overlay.overlay }
            AnchorChanges {
              target: content
              anchors.horizontalCenter: undefined
              anchors.verticalCenter: undefined
            }
            PropertyChanges { target: content; opacity: 0.85 }
          }
        }
      }
    }
  }
}
