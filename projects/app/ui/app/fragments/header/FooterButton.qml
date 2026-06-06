import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls

// Button that goes in different footers
Button {
  topInset: 0
  bottomInset: 0
  leftInset: 0
  rightInset: 0

  flat: true
  display: AbstractButton.TextBesideIcon
  icon.width: 25
  icon.height: 25
  font.capitalization: Font.Capitalize
}