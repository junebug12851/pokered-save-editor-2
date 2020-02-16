import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

MouseArea {
  hoverEnabled: true
  onContainsMouseChanged: framesEdit.menuBtnVisible = containsMouse
  width: childRow.implicitWidth
  height: childRow.implicitHeight

  Row {
    id: childRow
    anchors.fill: parent
    spacing: 5

    DaysEdit {}
    PlaytimeDivider {}

    HoursEdit {}
    PlaytimeDivider {}

    MinutesEdit {}
    PlaytimeDivider {}

    SecondsEdit {}
    PlaytimeDivider {}

    FramesEdit {
      id: framesEdit
    }
  }
}
