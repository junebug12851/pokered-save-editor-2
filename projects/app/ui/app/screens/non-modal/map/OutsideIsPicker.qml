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
  OutsideIsPicker.qml -- "Outside is: Pallet Town".

  ⭐ THE MOST CONSEQUENTIAL WARP BYTE IN THE SAVE, and until 2026-07-14 it was on no warp screen at
  all. It is `wLastMap` (save 0x2611), and it lives in WorldGeneral rather than with the warps, which
  is exactly why nobody editing warps ever found it.

  What it does: **every building's exit warp has destination `$FF`**, and `$FF` does not name a map.
  It means *"put me back on whatever map I last stood on outdoors"* -- and this byte is that map.
  Walk out of Red's house and you are in Pallet Town **because this byte says Pallet Town**.

  Why it is in the TOOLBAR and not in a panel: changing it **re-labels every "back outside" door on
  the canvas at once**, live. A control that changes what a dozen other things MEAN is not a control
  you bury three clicks deep -- and watching the chips re-read as you scroll the list is the thing
  that makes the byte make sense to a person.

  ⚠️ Twilight reached for this as "From map". The byte actually *called* that -- `wWarpedFromWhichMap`
  -- is **dead**: the game writes it on every warp and nothing, anywhere, reads it. This is the one
  she meant. See notes/reference/warps.md §4.

  Writing it writes ONE byte. (tst_warps: `lastMap_writesOneByte`.)
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  implicitWidth: trigger.implicitWidth
  implicitHeight: 26

  /// Drivable by name, for the DEBUG harness and the screenshot review. (reference/dev-harness.md)
  property bool openState: false
  onOpenStateChanged: openState ? pop.open() : pop.close()

  /// How many doors on this map actually say "back outside" -- i.e. how many this button governs.
  /// Bound through the map id so it re-asks; a bare warpList() call would answer once.
  readonly property int returnDoors: {
    brg.map.mapInd;   // a dependency, deliberately -- re-ask when the map changes
    const list = brg.map.warpList();
    let n = 0;
    for (let i = 0; i < list.length; i++)
      if (list[i].isReturn)
        n++;
    return n;
  }

  // The wordy "Outside is Pallet Town ×3" chip is gone (Twilight: "Pallet Town littered all over the
  // bar"). The WARP button is a ⇄ icon; the destination and the door count live in its tooltip and
  // its dropdown, where "Pallet Town" is said once instead of a third time.
  MapBarButton {
    id: trigger
    anchors.fill: parent

    glyph: "⇄"
    open: root.openState
    onToggle: root.openState = !root.openState

    tip: qsTr("Outside is: %1").arg(brg.map.lastMapName)
         + (root.returnDoors > 0
              ? "\n\n" + qsTr("%n “back outside” warp(s) on this map lead here.", "", root.returnDoors)
              : "\n\n" + qsTr("No warp here leads back outside — but it still sets where you come out "
                              + "of the next building."))
  }

  // ── The drop-down ───────────────────────────────────────────────────────────────────────────
  Popup {
    id: pop

    y: root.height + 4
    width: 300
    padding: 10

    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: Rectangle {
      color: "#ffffff"
      radius: 6
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 8

      RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Text {
          text: qsTr("Outside is…")
          font.pixelSize: 11
          font.bold: true
          color: brg.settings.textColorMid
        }

        Item { Layout.fillWidth: true }

        // The screen's ONE allowed "?" -- the panel-title info icon. Don't litter these; the mark
        // only means anything while it is rare. (reference/ui-patterns.md)
        MapInfoIcon {
          text: qsTr("A warp that says “back outside” doesn't name a map — it means “put me back on "
                     + "whatever map I was last standing on outdoors”. This is that map.\n\n"
                     + "It's why walking out of Red's house puts you in Pallet Town. Change it and "
                     + "every “back outside” warp on this map now leads somewhere else.")
        }
      }

      ComboBox {
        id: combo
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        font.pixelSize: 12

        model: brg.map.mapList()
        textRole: "name"
        valueRole: "ind"

        currentIndex: {
          const list = model;
          for (let i = 0; i < list.length; i++)
            if (list[i].ind === brg.map.lastMap)
              return i;
          return -1;
        }

        // ONE byte. (tst_warps: lastMap_writesOneByte.)
        onActivated: brg.map.lastMap = currentValue

        // Same grouped delegate as the map picker beside it -- 248 names in a flat list is a wall.
        delegate: ItemDelegate {
          required property var modelData
          required property int index

          width: combo.width
          height: (modelData.group !== "" ? 20 : 0) + 26
          highlighted: combo.highlightedIndex === index

          contentItem: ColumnLayout {
            spacing: 0

            Text {
              visible: modelData.group !== ""
              Layout.fillWidth: true
              text: modelData.group
              font.pixelSize: 10
              font.bold: true
              color: brg.settings.textColorMid
            }

            RowLayout {
              Layout.fillWidth: true
              spacing: 6

              Text {
                text: modelData.ind
                font.pixelSize: 10
                font.family: "monospace"
                color: brg.settings.textColorMid
                Layout.minimumWidth: 22
              }

              Text {
                Layout.fillWidth: true
                text: modelData.name
                font.pixelSize: 12
                color: brg.settings.textColorDark
                elide: Text.ElideRight
              }
            }
          }
        }
      }

      // What it governs, said plainly -- including when the answer is "nothing on this map", which is
      // an honest and useful thing to be told rather than a control that appears to do nothing.
      Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: root.returnDoors > 0
              ? qsTr("%n warp(s) on this map lead back outside — they all go here.", "", root.returnDoors)
              : qsTr("No warp on this map leads back outside, so changing this won't alter anything "
                     + "you can see from here. It still matters: it's where the game will put you "
                     + "when you next leave a building.")
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }

      Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: brg.settings.dividerColor
      }

      // ── Wake up at… (wLastBlackoutMap) ────────────────────────────────────────────────────────
      //
      // Its natural partner, and it is here for the same reason: it is a "where does the world put
      // me" byte, it lives in WorldGeneral too, and nobody editing warps has ever been able to see
      // it. Blacking out, DIG and an ESCAPE ROPE all land on it.
      RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Text {
          text: qsTr("Wake up at…")
          font.pixelSize: 11
          font.bold: true
          color: brg.settings.textColorMid
        }
      }

      ComboBox {
        id: blackoutCombo
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        font.pixelSize: 12

        model: brg.map.mapList()
        textRole: "name"
        valueRole: "ind"

        currentIndex: {
          const list = model;
          for (let i = 0; i < list.length; i++)
            if (list[i].ind === brg.map.lastBlackoutMap)
              return i;
          return -1;
        }

        onActivated: brg.map.lastBlackoutMap = currentValue

        delegate: ItemDelegate {
          required property var modelData
          required property int index

          width: blackoutCombo.width
          height: (modelData.group !== "" ? 20 : 0) + 26
          highlighted: blackoutCombo.highlightedIndex === index

          contentItem: ColumnLayout {
            spacing: 0

            Text {
              visible: modelData.group !== ""
              Layout.fillWidth: true
              text: modelData.group
              font.pixelSize: 10
              font.bold: true
              color: brg.settings.textColorMid
            }

            RowLayout {
              Layout.fillWidth: true
              spacing: 6

              Text {
                text: modelData.ind
                font.pixelSize: 10
                font.family: "monospace"
                color: brg.settings.textColorMid
                Layout.minimumWidth: 22
              }

              Text {
                Layout.fillWidth: true
                text: modelData.name
                font.pixelSize: 12
                color: brg.settings.textColorDark
                elide: Text.ElideRight
              }
            }
          }
        }
      }

      Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Blacking out, digging out with DIG, and using an ESCAPE ROPE all bring you here.")
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }
    }

    onOpenedChanged: root.openState = pop.opened
  }
}
