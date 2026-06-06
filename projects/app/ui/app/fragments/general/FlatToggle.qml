import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// Flat, square toggle button: no Material elevation/shadow, no rounded corners.
// Filled with the accent when `active`, outlined otherwise. Used for the
// keyboard's Outdoor / Grid-Tileset toggles and the quick-edit Name/Example
// toggle so they all read the same.
Button {
  id: ftb

  property bool active: false

  flat: true
  font.capitalization: Font.Capitalize
  font.pixelSize: 12
  Material.elevation: 0

  topPadding: 9
  bottomPadding: 9
  leftPadding: 5
  rightPadding: 5

  background: Rectangle {
    radius: 0
    border.width: 1
    border.color: brg.settings.accentColor
    color: ftb.active
           ? brg.settings.accentColor
           : (ftb.hovered ? Qt.lighter(brg.settings.accentColor, 1.65) : "transparent")
  }

  contentItem: Text {
    text: ftb.text
    font: ftb.font
    color: ftb.active ? brg.settings.textColorLight : brg.settings.textColorDark
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }
}
