import QtQuick 2.14

import "../../common/Style.js" as Style

MouseArea {
  property color refColor: Style.accentColor
  property color refDefColor: refColor
  property color curColor: refDefColor

  hoverEnabled: true

  onEntered: curColor = Qt.darker(refColor, 1.10)
  onExited: curColor = refDefColor
  onPressed: curColor = Qt.darker(refColor, 1.40)
  onReleased: curColor = (containsMouse)
              ? Qt.darker(refColor, 1.10)
              : refDefColor
}
