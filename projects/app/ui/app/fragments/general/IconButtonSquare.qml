import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// Flat icon button with a TIGHT, rectangular hover/press highlight (just around
// the icon) instead of Material's wide rounded button background. Used for all
// the ⋮ menu buttons so they read consistently and don't look like big pills.
Button {
  id: control
  display: AbstractButton.IconOnly
  icon.width: 15
  icon.height: 15
  flat: true
  topInset: 0
  bottomInset: 0
  leftInset: 0
  rightInset: 0
  padding: 6

  background: Rectangle {
    radius: 2
    color: control.down
           ? Qt.rgba(0, 0, 0, 0.16)
           : control.hovered
             ? Qt.rgba(0, 0, 0, 0.08)
             : "transparent"
  }
}
