// SegBtn.qml -- one segment of a connected action-button "combo" group (the app's
// randomize / clear / reset / max icon buttons). Flat, icon-only, with a square
// hover/press fill, a hairline left divider (except the first), and rounded outer
// corners on the first/last segments so a group reads as one pill. Tinted like the
// app's other icons.
//
// Shared control: the Pokemon-details tabs (Overview / Moves / DV-EV) AND the trainer
// card all use this, so every action button in the app is IDENTICAL. (Previously this
// was copy-pasted inline in three tabs.) Place inside a RowLayout -- it fills the group
// height via Layout.fillHeight; set first/last on the end segments for corner rounding.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
  id: seg
  property bool first: false
  property bool last: false
  property string tip: ""
  flat: true
  display: AbstractButton.IconOnly
  topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
  padding: 7
  icon.color: brg.settings.textColorDark
  Layout.fillHeight: true
  Layout.minimumHeight: 0
  background: Rectangle {
    color: seg.down ? Qt.rgba(0, 0, 0, 0.16)
           : seg.hovered ? Qt.rgba(0, 0, 0, 0.08)
           : "transparent"
    topLeftRadius: seg.first ? 4 : 0
    bottomLeftRadius: seg.first ? 4 : 0
    topRightRadius: seg.last ? 4 : 0
    bottomRightRadius: seg.last ? 4 : 0
    Rectangle {
      visible: !seg.first
      anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
      width: 1
      color: Qt.rgba(0, 0, 0, 0.15)
    }
  }
  MainToolTip { text: seg.tip }
}
