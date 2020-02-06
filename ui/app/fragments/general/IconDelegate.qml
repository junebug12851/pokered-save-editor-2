import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

Button {
  property int cellSize: brg.settings.iconViewCellSize

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

  //onClicked: root.changeScreen(page)
}
