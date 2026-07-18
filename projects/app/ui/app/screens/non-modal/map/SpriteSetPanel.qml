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
  ///
  /// ⚠️ It says something DIFFERENT indoors, because indoors the truth is different: there is no
  /// sprite set at all. (2026-07-13; the disassembly is in MapModel::vramPictures.)
  readonly property string panelInfo: brg.map.mapIsIndoors()
    ? qsTr("⚠️ This is an INDOOR map, and indoor maps don't use a sprite set.\n\n"
           + "The game loads each character's own artwork instead — so anybody can go anywhere in "
           + "here, until the video memory runs out (10 walking characters, 2 still ones).\n\n"
           + "These eleven bytes are just what was cached from the last outdoor map you were on. "
           + "The game ignores them completely in here.")
    : qsTr("The eleven sprite pictures the game loads for this map — nine that walk, two that don't. "
           + "Outdoors, every character on the map has to come out of this set.\n\n"
           + "It's a cache: the game rebuilds it from the cartridge every time it loads a save, so "
           + "editing it here won't change what you see in-game.")

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

  // ⭐ THE WHOLE PANEL IS A USELESS EDIT (leadership, 2026-07-18: *"the whole sprite set panel can
  // be hidden behind useless"*): the sprite cache is the one block the game THROWS AWAY on every
  // load (`LoadMapData` zeroes the id, `InitOutsideMapSprites` recomputes the lot -- see
  // reference/sprite-sets.md). Behind the toolbar's "!", with one line saying why.
  Label {
    anchors.centerIn: parent
    width: parent.width - 40
    visible: !brg.map.showScratch
    wrapMode: Text.Wrap
    horizontalAlignment: Text.AlignHCenter
    font.pixelSize: 11
    opacity: 0.55
    text: qsTr("The game rebuilds this whole cache every time it loads your save — nothing here "
               + "survives a Continue.\n\nTurn on “Useless edits” (the ! button in the toolbar) "
               + "to edit it anyway.")
  }

  ScrollView {
    anchors.fill: parent
    anchors.margins: 12
    clip: true
    visible: brg.map.showScratch

    ColumnLayout {
      // 12px of margin each side, PLUS the 16px overlay-scrollbar lane. @see ui-patterns.md
      width: panel.width - 24 - 16
      spacing: 10

      // ── ONE NOTICE, AT THE TOP, FOR THE WHOLE PANEL ──────────────────────────────────────────
      //
      // ⚠️ Twilight took this panel apart and she was right to:
      //
      //   > *"What is 'This map's set — Pallet and Viridian'? 'Load this map's set', greyed-out text
      //   > — is there any point to this at all? Is the set different from the eleven, are they
      //   > together? The second one doesn't have an exclamation point, so does that mean it's
      //   > different from 'The set'? The 'the' before it sounds dumb — call both of them something
      //   > else."*
      //
      // Every one of those is the panel's fault. There were three groups that looked like three
      // unrelated things, one of them wearing a "!" and the others not — as if some of it survived a
      // load and some didn't. **It is all one cache and ALL of it is overwritten.** So the notice is
      // said ONCE, here, for the panel, and the groups below are just the parts of it.
      Rectangle {
        Layout.fillWidth: true
        radius: 6
        color: Qt.rgba(1, 0.84, 0.31, 0.15)
        border.width: 1
        border.color: "#ffd54f"
        implicitHeight: topRow.implicitHeight + 14

        RowLayout {
          id: topRow
          anchors.fill: parent
          anchors.margins: 7
          spacing: 6

          MapWarnIcon {
            Layout.alignment: Qt.AlignTop
            text: qsTr("Every value on this panel is one the game works out again when it loads your "
                       + "save. They're yours to edit; they just won't survive Continue.")
          }

          Label {
            Layout.fillWidth: true
            text: brg.map.mapIsIndoors()
                    ? qsTr("Indoor maps don't use a sprite set at all — the game loads each "
                           + "character's own artwork. Nothing on this panel does anything here.")
                    : qsTr("The game rebuilds all of this from the cartridge every time it loads "
                           + "your save.")
            wrapMode: Text.Wrap
            font.pixelSize: 10
          }
        }
      }

      // ── What the save is carrying ────────────────────────────────────────────────────────────
      //
      // Named for what it IS -- the save's copy -- rather than "The set", which said nothing and
      // implied it was a different thing from the eleven pictures below it. It is not: picking here
      // fills those eleven.
      Group {
        title: qsTr("Cached set")
        hint: qsTr("Picking one fills the eleven pictures below.")

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

      // ── What the CARTRIDGE would load here ───────────────────────────────────────────────────
      //
      // ⚠️ HIDDEN INDOORS. Twilight: *"'Load this map's set', greyed-out text — is there any point to
      // this at all?"* Indoors: no, there is not. The map has no set, the button can never do
      // anything, and a permanently-disabled control that can never become enabled is furniture.
      // Outdoors it is the useful thing on this panel: it is what the console will put here anyway,
      // and the button gets you there in one click.
      Group {
        visible: brg.map.mapHasSpriteSet

        title: qsTr("What this map really loads")
        hint: qsTr("Out of the cartridge. This is what the game will put in the cache anyway.")

        Text {
          Layout.fillWidth: true
          text: brg.map.mapSpriteSetName
          font.pixelSize: 12
          font.bold: true
          color: brg.settings.textColorDark
          wrapMode: Text.WordWrap
        }

        Text {
          Layout.fillWidth: true
          visible: !brg.map.spriteSetMatchesMap
          text: qsTr("The save is carrying a different one.")
          font.pixelSize: 10
          color: "#8a6d00"
          wrapMode: Text.WordWrap
        }

        Button {
          Layout.fillWidth: true
          flat: true
          font.pixelSize: 11
          enabled: !brg.map.spriteSetMatchesMap
          text: qsTr("Copy it into the save")
          onClicked: brg.map.applyMapSpriteSet()
        }
      }

      // ── The eleven slots ─────────────────────────────────────────────────────────────────────
      //
      // "The eleven" -- which, as Twilight put it, "sounds dumb", and worse: it read as a *different
      // thing* from the set above it. It isn't. It IS the set: these are the eleven pictures that set
      // is made of, and picking a set up there fills them in down here.
      Group {
        title: qsTr("Cached pictures")
        hint: qsTr("The eleven the set is made of. 1–9 walk · 10 and 11 stand still.")

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
