import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// FlatToggle.qml --
// Flat, square toggle button: no Material elevation/shadow, no rounded corners.
// Filled with the accent when `active`, outlined otherwise. Used for the
// keyboard's Outdoor / Grid-Tileset toggles and the quick-edit Name/Example
// toggle so they all read the same.
Button {
  id: ftb

  property bool active: false

  // Colors are overridable so the same toggle reads correctly on different
  // backgrounds. Defaults reproduce the original look exactly (accent fill /
  // dark-or-light text on a light surface) -- the keyboard toggles rely on
  // these. Header-bar instances (on the accent bar) invert them.
  property color toggleColor: brg.settings.accentColor                       ///< Border + fill-when-active.
  property color activeTextColor: brg.settings.textColorLight                ///< Label when On.
  property color inactiveTextColor: brg.settings.textColorDark               ///< Label when Off.
  property color hoverColor: Qt.lighter(brg.settings.accentColor, 1.65)      ///< Fill on hover (Off state).

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
    border.color: ftb.toggleColor
    color: ftb.active
           ? ftb.toggleColor
           : (ftb.hovered ? ftb.hoverColor : "transparent")
  }

  contentItem: Text {
    text: ftb.text
    font: ftb.font
    color: ftb.active ? ftb.activeTextColor : ftb.inactiveTextColor
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }
}
