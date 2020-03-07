import QtQuick 2.14
import QtCharts 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../general"
import "../../header"
import "./stats"

Rectangle {
  property PokemonBox boxData: null
  property PokemonBox partyData: null

  color: "transparent"

  Button {
    id: statsTglBtn;

    anchors.top: parent.top
    anchors.topMargin: 15
    anchors.horizontalCenter: parent.horizontalCenter

    text: "DV"
    width: font.pixelSize * 5
    flat: true

    onClicked: {
      if(text == "DV")
        text = "EV";
      else
        text = "DV";
    }
  }

  IconButtonSquare {
    anchors.left: statsTglBtn.right
    anchors.leftMargin: -5
    anchors.top: statsTglBtn.top
    anchors.topMargin: 0

    icon.width: 7
    icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
    icon.color: brg.settings.textColorDark

    onClicked: statsTglBtnMenu.open();

    Menu {
      id: statsTglBtnMenu
      MenuItem { text: "Close" }
    }
  }

  DvStatGroup {
    visible: statsTglBtn.text == "DV"

    anchors.top: statsTglBtn.bottom
    anchors.topMargin: 15
    anchors.left: parent.left
    anchors.leftMargin: 5
    anchors.right: parent.right
    anchors.rightMargin: 5
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 5
  }

  EvStatGroup {
    visible: statsTglBtn.text == "EV"

    anchors.top: statsTglBtn.bottom
    anchors.topMargin: 15
    anchors.left: parent.left
    anchors.leftMargin: 5
    anchors.right: parent.right
    anchors.rightMargin: 5
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 5
  }

  Row {
    anchors.bottom: parent.bottom
    anchors.left: parent.left

    ShadedBG {
      id: futureShiny

      HeaderText {
        text: "Future Shiny"
      }

      CheckBox {
        id: futureShinyEdit

        anchors.top: futureShiny.top
        anchors.topMargin: -5
        anchors.left: futureShiny.right
        anchors.leftMargin: 0

        onCheckedChanged: {
          if(checked)
            boxData.makeShiny();
          else
            boxData.unmakeShiny();
        }

        Component.onCompleted: checked = boxData.isShiny;

        Connections {
          target: boxData
          onDvChanged: futureShinyEdit.checked = boxData.isShiny;
        }
      }

      IconButtonSquare {
        anchors.left: futureShinyEdit.right
        anchors.leftMargin: -15
        anchors.top: futureShiny.top
        anchors.topMargin: -3
        icon.width: 7

        icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
        icon.color: brg.settings.textColorDark

        onClicked: futureShinyMenu.open();

        Menu {
          id: futureShinyMenu
          MenuItem {
            text: "Re-Roll Shiny";
            onTriggered: boxData.rollShiny();
          }
          MenuItem {
            text: "Re-Roll Non-Shiny";
            onTriggered: boxData.rollNonShiny();
          }
          MenuSeparator { }
          MenuItem {
            text: "Convert to Shiny"
            enabled: !boxData.isShiny
            onTriggered: boxData.makeShiny();
          }
          MenuItem {
            text: "Convert to Non-Shiny"
            enabled: boxData.isShiny
            onTriggered: boxData.unmakeShiny();
          }
          MenuSeparator { }
          MenuItem { text: "Close" }
        }
      }
    }
  }
}
