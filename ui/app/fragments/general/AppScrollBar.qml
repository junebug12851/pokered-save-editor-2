import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

ScrollBar {
  id: bar
  policy: ScrollBar.AsNeeded

  contentItem: Rectangle {
    width: 6
    implicitHeight: 100
    radius: 100
    color: bar.pressed
           ? Qt.lighter(brg.settings.textColorMid, 1.75)
           : Qt.lighter(brg.settings.textColorMid, 1.50)
  }
}
