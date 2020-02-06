import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/header"

Page {
  anchors.fill: parent
  property int cellSize: 150

  GridView {
    anchors.fill: parent

    cellWidth: cellSize
    cellHeight: cellSize
    clip: true

    model: ListModel {
      ListElement {
        name: "Trainer Card"
        iconSrc: "qrc:/assets/icons/poke-go/hat.svg"
        page: "trainerCard"
      }
      ListElement {
        name: "Pokedex"
        iconSrc: "qrc:/assets/icons/poke-go/pokedex.svg"
        page: ""
      }
      ListElement {
        name: "Bag"
        iconSrc: "qrc:/assets/icons/poke-go/backpack.svg"
        page: ""
      }
      ListElement {
        name: "Party"
        iconSrc: "qrc:/assets/icons/poke-go/pikachu-2.svg"
        page: ""
      }
      ListElement {
        name: "Rival"
        iconSrc: "qrc:/assets/icons/poke-go/player.svg"
        page: ""
      }
      ListElement {
        name: "Storage"
        iconSrc: "qrc:/assets/icons/poke-go/smartphone.svg"
        page: ""
      }
      ListElement {
        name: "Maps"
        iconSrc: "qrc:/assets/icons/poke-go/map.svg"
        page: ""
      }
      ListElement {
        name: "Options"
        iconSrc: "qrc:/assets/icons/poke-go/gear.svg"
        page: ""
      }
      ListElement {
        name: "Hall of Fame"
        iconSrc: "qrc:/assets/icons/poke-go/crown.svg"
        page: ""
      }
      ListElement {
        name: "Event Pokemon"
        iconSrc: "qrc:/assets/icons/poke-go/mew.svg"
        page: ""
      }
    }

    delegate: Button {
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
  }

  footer: AppFooterBtn1 {}
}
