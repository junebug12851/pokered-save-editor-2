// SegToggle.qml -- an INDEPENDENT on/off toggle segment of a connected "combo"
// group. Same visual language as the Pokemon-details SegSel/SegBtn combos (flat;
// fills with the accent when `active`, otherwise a hover/press wash; a hairline
// left divider unless it's the first segment or it's active; per-corner rounding
// on the end segments so the group reads as one pill). Unlike SegSel -- whose
// segments are mutually exclusive -- each SegToggle is its own boolean: `active`
// binds to external state and `onClicked` flips it, so the segment always mirrors
// the underlying value and never drifts. Place inside a RowLayout and set
// first/last on the end segments.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
  id: stog
  property bool active: false
  property bool first: false
  property bool last: false
  property string tip: ""
  flat: true
  display: AbstractButton.TextOnly
  topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
  leftPadding: 12; rightPadding: 12; topPadding: 0; bottomPadding: 0
  Layout.fillHeight: true
  Layout.minimumHeight: 0
  font.pixelSize: 12
  contentItem: Text {
    text: stog.text
    font: stog.font
    color: stog.active ? brg.settings.textColorLight : brg.settings.textColorMid
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }
  // Per-corner radius (Qt 6.7+) so the fill on an end segment follows the group's
  // rounded corners instead of showing a flat square edge.
  background: Rectangle {
    color: stog.active ? brg.settings.accentColor
           : stog.down ? Qt.rgba(0, 0, 0, 0.16)
           : stog.hovered ? Qt.rgba(0, 0, 0, 0.08)
           : "transparent"
    topLeftRadius: stog.first ? 4 : 0
    bottomLeftRadius: stog.first ? 4 : 0
    topRightRadius: stog.last ? 4 : 0
    bottomRightRadius: stog.last ? 4 : 0
    Rectangle {
      visible: !stog.first && !stog.active
      anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
      width: 1
      color: Qt.rgba(0, 0, 0, 0.15)
    }
  }
  MainToolTip { text: stog.tip }
}
