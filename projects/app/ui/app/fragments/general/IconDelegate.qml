// IconDelegate.qml -- one icon-over-label tile in an IconsView grid.
//
// A flat square Button (cellSize) showing the model's icon above its capitalized
// name; clicking navigates via brg.router.changeScreen(page). Used for the Home
// menu grid.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
  property int cellSize: 150

  text: name
  width: cellSize
  height: cellSize

  icon.source: iconSrc
  icon.width: 75
  icon.height: 75
  icon.color: "transparent"

  font.pixelSize: 15
  font.capitalization: Font.Capitalize

  flat: true
  display: AbstractButton.TextUnderIcon
  padding: 20

  onClicked: brg.router.changeScreen(page)
}
