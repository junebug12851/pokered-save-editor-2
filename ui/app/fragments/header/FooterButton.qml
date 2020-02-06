import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

// Button that goes in different footers
Button {
  topInset: -20
  bottomInset: -20

  flat: true
  display: AbstractButton.TextBesideIcon
  icon.width: 25
  icon.height: 25
  font.capitalization: Font.Capitalize
  font.pixelSize: 20
}
