import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/header"
import "../../fragments/general"
import "../../fragments/screens/home"

Page {
  id: page

  ComboBox {
    textRole: "itemName"
    valueRole: "itemInd"

    font.capitalization: Font.Capitalize
    font.pixelSize: 14
    flat: true
    model: brg.itemsModel

    width: font.pixelSize * 15

    background: Rectangle {
      implicitWidth: 120
      implicitHeight: 40
      border.color: control.pressed ? "#17a81a" : "#21be2b"
      border.width: control.visualFocus ? 2 : 1
      radius: 2
    }
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: brg.file.data.randomizeExpansion()
  }
}
