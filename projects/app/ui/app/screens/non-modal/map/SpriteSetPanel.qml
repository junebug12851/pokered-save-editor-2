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

  These are your save's bytes and you may edit every one of them; they are simply bytes the game will
  overwrite before it ever reads them.

  Indoors, the game skips the whole routine -- so a save made in a building keeps whatever set was
  loaded in the last outdoor map. That is not a bug in the save; it is what a console holds too.

  ⚠️ **THE WALL OF TEXT IS GONE.** All of the above used to be printed *in the panel*, two paragraphs
  over the controls plus a blurb under every group heading, and you had to read past the lot every
  time to reach a combo box. Twilight, 2026-07-13: *"Remove all the text below Sprite Set — it's way
  too much to read; condense it somehow and put it somewhere like a tooltip or hint."*

  So the words moved to two marks:

    * the **?** in the title (`panelInfo`) -- what this is. Hover it if you want it. (MapInfoIcon)
    * the yellow **!** on the cache      -- the game overwrites this on load. (MapWarnIcon)

  Don't put prose back in here.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panel

  // The dock owns the frame and the title bar. A panel is its content.
  color: "transparent"

  /// The panel's "?" — everything this panel used to print at you. (MapDock puts it in the title.)
  readonly property string panelInfo: qsTr(
    "The eleven sprite pictures the game had loaded for this map — nine that walk, two that don't. "
    + "Every character on the map has to come out of this set.\n\n"
    + "It's a cache: the game rebuilds it from the map you're standing on every time it loads a "
    + "save, so editing it here won't change what you see in-game.")

  /// A group box. A title, and whatever you put in it — the per-group paragraph is GONE (it is what
  /// made this panel a wall of text). If a group needs a caveat, it gets a `hint` or a MapWarnIcon.
  component Group: Rectangle {
    id: grp
    default property alias content: inner.data
    property string title: ""

    /// One SHORT line, under the title. Not a paragraph. If it wants to be a paragraph, it belongs
    /// in `panelInfo` behind the "?".
    property string hint: ""

    /// The game overwrites these bytes when it loads the save. Shows the yellow "!" beside the title.
    property string overwritten: ""

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

      RowLayout {
        Layout.fillWidth: true
        spacing: 5
        visible: grp.title !== ""

        Text {
          text: grp.title
          font.pixelSize: 12
          font.bold: true
          color: brg.settings.textColorDark
        }

        MapWarnIcon {
          visible: grp.overwritten !== ""
          text: grp.overwritten
        }

        Item { Layout.fillWidth: true }
      }

      Text {
        Layout.fillWidth: true
        visible: grp.hint !== ""
        text: grp.hint
        font.pixelSize: 10
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

      // (Two paragraphs of explanation opened this panel. They are the "?" in the title now --
      // Twilight, 2026-07-13. Do not put them back.)

      // ── Which set ────────────────────────────────────────────────────────────────────────────
      Group {
        title: qsTr("The set")
        overwritten: qsTr("The game rebuilds this from the map you're standing on every time it "
                          + "loads your save — so whatever you set here, it won't survive Continue.")

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
        hint: brg.map.mapHasSpriteSet
              ? ""
              : qsTr("Indoors — there isn't one. This is the last outdoor map's.")

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
        hint: qsTr("1–9 walk · 10 and 11 stand still")

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
