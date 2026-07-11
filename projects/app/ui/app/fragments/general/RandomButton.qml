// RandomButton.qml -- the app-wide "randomize this field" control: a rounded, 1px-
// bordered group holding a dice SegBtn, and -- when `showClear` is set -- a second
// trash SegBtn beside it so the group reads as one [dice | trash] pill. This is the
// SAME control the Pokemon-details editor uses for its randomize/clear actions
// (identical SegBtn, identical 30px rounded border group -- see OverviewTab's /
// StatsTab's action groups), so the buttons look the same on every screen. One
// source of truth, no per-screen sizing.
//
// Anchors just off the RIGHT edge of its parent field, vertically centred. Emits
// randomize() on the dice and clear() on the trash. `showClear` defaults false, so
// fields that only randomize (Starter, Player ID) stay a single rounded dice
// exactly as before. (Replaced the old hover-reveal "..." overflow menu.)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Rectangle {
  id: top
  signal randomize();
  signal clear();
  property alias tip: seg.tip

  // Set true to add the trailing trash segment (the [dice | trash] combined look).
  property bool showClear: false
  property string clearTip: qsTr("Clear this field.")

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
    // Same dice segment as the Pokemon-details action groups. It rounds its right
    // corners only when it's the lone segment (no clear button).
    SegBtn {
      id: seg
      first: true
      last: !top.showClear
      icon.width: 18
      icon.height: 18
      icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
      tip: qsTr("Randomize this field.")
      onClicked: top.randomize();
    }
    // Optional trailing clear/delete segment. A !visible Layout item collapses to
    // zero width, so the group narrows back to just the dice when showClear is off.
    SegBtn {
      id: clearSeg
      visible: top.showClear
      last: true
      icon.width: 15
      icon.height: 15
      icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
      tip: top.clearTip
      onClicked: top.clear();
    }
  }
}
