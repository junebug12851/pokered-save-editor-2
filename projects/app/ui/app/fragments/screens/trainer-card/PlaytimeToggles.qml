// PlaytimeToggles.qml -- the Enabled and Paused toggles shown below the playtime
// clock. They are two INDEPENDENT states (the clock can be enabled and paused, or
// neither), so they are rendered as two SEPARATE rounded toggle buttons with a gap
// between them -- deliberately NOT one connected segmented group, which would imply a
// mutually-exclusive "one or the other" choice. Each pill's `active` binds to the
// underlying bit and flips it on click (same rounded-border look as RandomButton).
//
//   Enabled -- reflects the clock NOT being maxed (playtime.clockMaxed). On = live.
//   Paused  -- reflects the game NOT counting playtime (area.general.countPlaytime).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

RowLayout {
  id: top
  spacing: 10

  property int pillH: 26

  // --- Enabled ---
  Rectangle {
    Layout.preferredHeight: top.pillH
    implicitHeight: top.pillH
    implicitWidth: enGrp.implicitWidth
    radius: 4
    color: "transparent"
    border.width: 1
    border.color: Qt.rgba(0, 0, 0, 0.18)
    clip: true
    RowLayout {
      id: enGrp
      anchors.fill: parent
      spacing: 0
      SegToggle {
        first: true
        last: true
        text: qsTr("Enabled")
        active: !brg.file.data.dataExpanded.world.other.playtime.clockMaxed
        onClicked: brg.file.data.dataExpanded.world.other.playtime.clockMaxed =
                   !brg.file.data.dataExpanded.world.other.playtime.clockMaxed
        tip: qsTr("Whether the playtime clock is running. Off = the clock is maxed/stopped.")
      }
    }
  }

  // --- Paused ---
  Rectangle {
    Layout.preferredHeight: top.pillH
    implicitHeight: top.pillH
    implicitWidth: pauGrp.implicitWidth
    radius: 4
    color: "transparent"
    border.width: 1
    border.color: Qt.rgba(0, 0, 0, 0.18)
    clip: true
    RowLayout {
      id: pauGrp
      anchors.fill: parent
      spacing: 0
      SegToggle {
        first: true
        last: true
        text: qsTr("Paused")
        active: !brg.file.data.dataExpanded.area.general.countPlaytime
        onClicked: brg.file.data.dataExpanded.area.general.countPlaytime =
                   !brg.file.data.dataExpanded.area.general.countPlaytime
        tip: qsTr("Whether the game is currently counting playtime.")
      }
    }
  }
}
