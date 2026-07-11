// PlaytimeGroup.qml -- a light titled "grouping box" that gathers everything to do
// with the playtime clock on the trainer card: a "Playtime" title, the clock digit
// fields with their [dice | trash] randomize/clear button, and the [Enabled | Paused]
// toggle pair sitting BELOW the fields. A subtle rounded border ties the whole cluster
// together so it reads as one labelled group (see ui-patterns.md).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

Rectangle {
  id: grp

  // Shared field height, forwarded to the clock digits.
  property int fieldH: 28

  // Inner padding.
  property int padH: 14
  property int padV: 9

  radius: 6
  color: "transparent"
  border.width: 1
  border.color: Qt.rgba(0, 0, 0, 0.22)

  // Size to the content (the clock+button row is the widest thing).
  implicitWidth: Math.max(clockRow.width, toggles.implicitWidth) + 2 * grp.padH
  implicitHeight: grp.padV + title.implicitHeight + 8 + grp.fieldH + 8
                  + toggles.implicitHeight + grp.padV

  // ---- Title ----
  Text {
    id: title
    text: qsTr("Playtime")
    color: brg.settings.textColorMid
    font.pixelSize: 12
    font.bold: true
    font.capitalization: Font.Capitalize
    anchors.top: parent.top
    anchors.topMargin: grp.padV
    anchors.horizontalCenter: parent.horizontalCenter
  }

  // ---- Clock fields + [dice | trash] ---- (centered in the group)
  Item {
    id: clockRow
    anchors.top: title.bottom
    anchors.topMargin: 8
    anchors.horizontalCenter: parent.horizontalCenter
    height: grp.fieldH
    width: clock.width + 6 + rand.width

    PlaytimeEdit {
      id: clock
      fieldH: grp.fieldH
      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter
    }

    // The shared [dice | trash] control. Re-anchored to sit just right of the
    // clock (overriding RandomButton's default "off the parent's right edge").
    RandomButton {
      id: rand
      anchors.left: clock.right
      anchors.leftMargin: 6
      anchors.verticalCenter: clock.verticalCenter
      tip: qsTr("Randomize the playtime.")
      onRandomize: brg.file.data.dataExpanded.world.other.randomizePlaytime();
      showClear: true
      clearTip: qsTr("Clear the playtime to 0.")
      onClear: brg.file.data.dataExpanded.world.other.clearPlaytime();
    }
  }

  // ---- Enabled / Paused, below the fields (centered) ----
  PlaytimeToggles {
    id: toggles
    anchors.top: clockRow.bottom
    anchors.topMargin: 8
    anchors.horizontalCenter: parent.horizontalCenter
  }
}
