import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls

// FooterButton.qml -- a single flat text+icon button used inside the app footers.
//
// The shared button building-block for AppFooterBtn1/2/3. Zero insets so the
// buttons tile edge-to-edge across the footer; capitalized label beside the icon.
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