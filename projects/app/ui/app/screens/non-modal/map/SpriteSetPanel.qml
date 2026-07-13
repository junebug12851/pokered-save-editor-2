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

/*
  SpriteSetPanel.qml -- the SPRITE SET. (v1 called this "cached sprites", which is not wrong, but it
  never said what the thing was.)

  What it actually is, out of the game's own source (engine/overworld/map_sprites.asm; the full
  write-up is notes/reference/sprite-sets.md):

    * The Game Boy can only hold ELEVEN overworld sprite pictures in video memory at once. So every
      outdoor map names a SPRITE SET -- nine walking sprites and two still ones -- and the game loads
      that set's graphics when you arrive. Every NPC on the map must be drawn from it.
    * Your save keeps a copy: eleven picture ids, plus the id of the set they came from. Twelve bytes.
    * ⚠️ IT IS A CACHE, AND THE GAME THROWS IT AWAY. `LoadMapData` zeroes the set id every time it
      loads a map -- including the one it loads when you press CONTINUE -- and then re-reads the set
      from the map you are standing on. So nothing you change here changes the game.

  Which is exactly why the panel says so, at the top, in plain words. These are your save's bytes and
  you may edit them; they are simply bytes the game will overwrite before it ever reads them.

  Indoors, the game skips the whole routine -- so a save made in a building keeps whatever set was
  loaded in the last outdoor map. That is not a bug in the save; it is what a console holds too.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panel

  // The dock owns the frame and the title bar. A panel is its content.
  color: "transparent"

  component Group: Rectangle {
    id: grp
    default property alias content: inner.data
    property string title: ""
    property string blurb: ""

    Layout.fillWidth: true
    implicitHeight: col.implicitHeight + 16

    radius: 4
    color: "#ffffff"
    border.width: 1
    border.color: brg.settings.dividerColor

    ColumnLayout {
      id: col
      anchors.fill: parent
      anchors.margins: 8
      spacing: 6

      Text {
        visible: grp.title !== ""
        text: grp.title
        font.pixelSize: 12
        font.bold: true
        color: brg.settings.textColorDark
      }

      Text {
        Layout.fillWidth: true
        visible: grp.blurb !== ""
        text: grp.blurb
        font.pixelSize: 11
        color: brg.settings.textColorMid
        wrapMode: Text.WordWrap
      }

      ColumnLayout {
        id: inner
        Layout.fillWidth: true
        spacing: 6
      }
    }
  }

  ScrollView {
    anchors.fill: parent
    anchors.margins: 12
    clip: true

    ColumnLayout {
      width: panel.width - 24
      spacing: 10

      // ── What this is, and the one thing worth knowing about it ────────────────────────────────
      Text {
        Layout.fillWidth: true
        text: qsTr("The eleven sprite pictures the game had loaded for this map — nine that walk, "
                   + "two that don't. Every NPC here has to come out of this set.")
        font.pixelSize: 11
        color: brg.settings.textColorMid
        wrapMode: Text.WordWrap
      }

      Text {
        Layout.fillWidth: true
        text: qsTr("It's a cache. The game rebuilds it from the map you're standing on every time it "
                   + "loads a save, so editing it here won't change what you see in-game.")
        font.pixelSize: 10
        font.italic: true
        color: brg.settings.textColorMid
        wrapMode: Text.WordWrap
      }

      // ── Which set ────────────────────────────────────────────────────────────────────────────
      Group {
        title: qsTr("The set")
        blurb: qsTr("Which of the game's ten sprite sets these came from.")

        ComboBox {
          Layout.fillWidth: true
          Layout.preferredHeight: 30
          font.pixelSize: 11

          model: brg.map.spriteSetList()
          textRole: "name"
          valueRole: "ind"

          currentIndex: {
            const list = model;
            for (let i = 0; i < list.length; i++)
              if (list[i].ind === brg.map.spriteSetId)
                return i;
            return -1;   // a value no set has -- shown as it is, never corrected
          }

          // Picking a set fills all twelve bytes: the eleven pictures AND the id. That is what the
          // game does, so that is what the control does.
          onActivated: brg.map.applySpriteSet(currentValue)
        }

        // The save is holding something the game would never write there (a split id, or a set that
        // doesn't exist). A fact, in the same voice as everything else here.
        Text {
          Layout.fillWidth: true
          visible: brg.map.spriteSetId > 10
          text: brg.map.spriteSetName
          font.pixelSize: 10
          color: brg.settings.textColorMid
          wrapMode: Text.WordWrap
        }
      }

      // ── What the game would load here ────────────────────────────────────────────────────────
      Group {
        title: qsTr("This map's set")
        blurb: brg.map.mapHasSpriteSet
               ? qsTr("What the game loads when you walk in here.")
               : qsTr("Indoor maps don't have one — the game leaves whatever was cached alone, so "
                      + "this is the set from the last outdoor map you were on.")

        RowLayout {
          Layout.fillWidth: true
          visible: brg.map.mapHasSpriteSet
          spacing: 6

          Text {
            Layout.fillWidth: true
            text: brg.map.mapSpriteSetName
            font.pixelSize: 11
            font.bold: true
            color: brg.settings.textColorDark
            wrapMode: Text.WordWrap
          }
        }

        // The cache disagrees with the map. Not an error -- the game will fix it itself on the next
        // load -- so it says so quietly, and offers to do it now.
        Text {
          Layout.fillWidth: true
          visible: brg.map.mapHasSpriteSet && !brg.map.spriteSetMatchesMap
          text: qsTr("The cache is from a different set.")
          font.pixelSize: 10
          color: "#8a6d00"
          wrapMode: Text.WordWrap
        }

        Button {
          Layout.fillWidth: true
          visible: brg.map.mapHasSpriteSet
          flat: true
          font.pixelSize: 11
          enabled: !brg.map.spriteSetMatchesMap
          text: qsTr("Load this map's set")
          onClicked: brg.map.applyMapSpriteSet()
        }
      }

      // ── The eleven slots ─────────────────────────────────────────────────────────────────────
      Group {
        title: qsTr("The eleven")
        blurb: qsTr("Slots 1–9 walk. Slots 10 and 11 stand still — Pokéballs, boulders, that sort of "
                    + "thing.")

        Repeater {
          model: brg.map.cachedSprites()

          ColumnLayout {
            required property var modelData

            Layout.fillWidth: true
            spacing: 1

            Text {
              text: modelData.still
                    ? qsTr("%1 · still").arg(modelData.slot + 1)
                    : qsTr("%1").arg(modelData.slot + 1)
              font.pixelSize: 10
              color: brg.settings.textColorMid
            }

            ComboBox {
              Layout.fillWidth: true
              Layout.preferredHeight: 28
              font.pixelSize: 11

              model: brg.map.spriteList()
              textRole: "name"
              valueRole: "ind"

              currentIndex: {
                const list = model;
                for (let i = 0; i < list.length; i++)
                  if (list[i].ind === modelData.ind)
                    return i;
                return -1;
              }

              // One slot, one byte. The set id is deliberately NOT touched -- if you hand-edit a
              // slot, the save now holds a set that isn't quite the set it says it is, and that is
              // your business, not ours to tidy up.
              onActivated: brg.map.setCachedSprite(modelData.slot, currentValue)
            }
          }
        }
      }

      Item { Layout.preferredHeight: 8 }
    }
  }
}
