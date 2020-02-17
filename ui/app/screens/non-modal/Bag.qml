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
    id: control
    textRole: "itemName"
    valueRole: "itemInd"

    font.capitalization: Font.Capitalize
    font.pixelSize: 14
    flat: true
    model: brg.itemsModel

    width: font.pixelSize * 15

    delegate: ItemDelegate {
      width: control.width
      enabled: itemInd >= 0;

      contentItem: Text {
        text: itemName
        font: control.font
        color: (itemInd >= 0)
               ? brg.settings.textColorDark
               : brg.settings.textColorMid
        verticalAlignment: Text.AlignVCenter
      }

      highlighted: control.highlightedIndex === index
    }

    popup: Popup {
      y: control.height - 1
      width: control.width
      implicitHeight: contentItem.implicitHeight
      padding: 1

      contentItem: ListView {
        clip: true
        implicitHeight: contentHeight
        model: control.popup.visible ? control.delegateModel : null
        currentIndex: control.highlightedIndex

        ScrollBar.vertical: ScrollBar { }
      }
    }
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: brg.file.data.randomizeExpansion()
  }
}
