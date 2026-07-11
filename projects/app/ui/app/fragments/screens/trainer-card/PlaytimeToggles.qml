// PlaytimeToggles.qml -- the [Enabled | Paused] toggle pair shown just above the
// playtime clock. Two independent SegToggle segments in one rounded combo group,
// the same visual language as the Pokemon-details action combos.
//
//   Enabled -- reflects the clock NOT being maxed (playtime.clockMaxed). On = the
//              clock is live; off = maxed/stopped.
//   Paused  -- reflects the game NOT counting playtime (area.general.countPlaytime).
//              On = currently paused.
//
// Each segment's `active` binds to the underlying bit and `onClicked` flips it, so
// the toggles always mirror the save and never drift. (These replaced the two
// checkable items in the old Frames overflow menu.)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

Rectangle {
  id: top
  implicitHeight: 24
  implicitWidth: grp.implicitWidth
  radius: 4
  color: "transparent"
  border.width: 1
  border.color: Qt.rgba(0, 0, 0, 0.18)
  clip: true

  RowLayout {
    id: grp
    anchors.fill: parent
    spacing: 0

    SegToggle {
      first: true
      text: qsTr("Enabled")
      active: !brg.file.data.dataExpanded.world.other.playtime.clockMaxed
      onClicked: brg.file.data.dataExpanded.world.other.playtime.clockMaxed =
                 !brg.file.data.dataExpanded.world.other.playtime.clockMaxed
      tip: qsTr("Whether the playtime clock is running. Off = the clock is maxed/stopped.")
    }

    SegToggle {
      last: true
      text: qsTr("Paused")
      active: !brg.file.data.dataExpanded.area.general.countPlaytime
      onClicked: brg.file.data.dataExpanded.area.general.countPlaytime =
                 !brg.file.data.dataExpanded.area.general.countPlaytime
      tip: qsTr("Whether the game is currently counting playtime.")
    }
  }
}
