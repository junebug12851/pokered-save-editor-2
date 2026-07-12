import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// IconButtonSquare.qml --
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

  // OPT-IN "tray" chrome: a bordered, rounded, always-visible button face rather than
  // the default transparent-until-hovered one. Off by default, so every existing caller
  // is unchanged.
  //
  // It exists because a ROW of these looked broken: sitting next to controls that DO have
  // a background, the flat ones read as a different, unrelated kind of thing. Within one
  // row of buttons, either all of them have a face or none do.
  property bool trayed: false

  background: Rectangle {
    radius: control.trayed ? 4 : 2

    color: control.trayed
           ? (control.down
              ? Qt.lighter(brg.settings.dividerColor, 1.05)
              : control.hovered
                ? Qt.lighter(brg.settings.dividerColor, 1.22)
                : "#ffffff")
           : (control.down
              ? Qt.rgba(0, 0, 0, 0.16)
              : control.hovered
                ? Qt.rgba(0, 0, 0, 0.08)
                : "transparent")

    border.width: control.trayed ? 1 : 0
    border.color: brg.settings.dividerColor
  }
}
