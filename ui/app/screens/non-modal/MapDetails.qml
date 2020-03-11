import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top
  property int mapInd: 0

  GridView {
    id: view
    anchors.fill: parent
    property int cellSize: 100

    cellWidth: cellSize
    cellHeight: cellSize
    clip: true

    delegate: Button {
      property int cellSize: view.cellSize

      text: name
      width: cellSize
      height: cellSize

      icon.source: iconSrc
      icon.width: 45
      icon.height: 45
      icon.color: "transparent"

      font.pixelSize: 15
      font.capitalization: Font.Capitalize

      flat: true
      display: AbstractButton.TextUnderIcon
      padding: 20

      //onClicked: brg.router.changeScreen(page)
    }

    model: ListModel {
      ListElement {
        name: "Loaded"
        iconSrc: "qrc:/assets/icons/fontawesome/map.svg"
        page: ""
      }

      ListElement {
        name: "General"
        iconSrc: "qrc:/assets/icons/fontawesome/cog.svg"
        page: ""
      }

      ListElement {
        name: "Events"
        page: ""
        iconSrc: "qrc:/assets/icons/fontawesome/globe-americas.svg"
      }

      ListElement {
        name: "Trades"
        iconSrc: "qrc:/assets/icons/fontawesome/hand-holding-heart.svg"
        page: ""
      }

      ListElement {
        name: "Missables"
        iconSrc: "qrc:/assets/icons/fontawesome/eye-slash.svg"
        page: ""
      }
    }

    ScrollBar.vertical: ScrollBar {}
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    //onBtn1Clicked: brg.file.data.randomizeExpansion()
  }
}
