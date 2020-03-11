import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top
  property int mapInd: 0

  Text {
    anchors.centerIn: parent
    text: mapInd
  }
}
