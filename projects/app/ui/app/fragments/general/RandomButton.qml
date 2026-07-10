// RandomButton.qml -- the app-wide "randomize this field" control: a rounded, 1px-
// bordered group holding a single dice SegBtn. This is the SAME control the
// Pokemon-details editor uses for its randomize actions (identical SegBtn, identical
// 30px rounded border group -- see OverviewTab's action groups), so the randomize
// button looks the same on every screen. One source of truth, no per-screen sizing.
//
// Anchors just off the RIGHT edge of its parent field, vertically centred. Emits
// randomize() on click. (Replaced the old hover-reveal "..." overflow menu.)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Rectangle {
  id: top
  signal randomize();
  property alias tip: seg.tip

  anchors.verticalCenter: parent.verticalCenter
  anchors.left: parent.right
  anchors.leftMargin: 6

  implicitHeight: 30
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
    // Same dice segment as the Pokemon-details action groups.
    SegBtn {
      id: seg
      first: true
      last: true
      icon.width: 18
      icon.height: 18
      icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
      tip: qsTr("Randomize this field.")
      onClicked: top.randomize();
    }
  }
}
