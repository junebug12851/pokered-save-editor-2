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

    MainToolTip {
      text: (statsTglBtn.text == "DV")
            ? "These are set on capture and never change in-game, they help affect the stats making each Pokemon unique. HP is a 5th DV that's calculated from the other DV."
            : "These are zero on capture and increase after every battle until maxed out. They slowly increase based on each opponent defeated and help make each Pokemon unique."
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

    onClicked: (statsTglBtn.text === "DV")
               ? statsTglBtnMenuDV.open()
               : statsTglBtnMenuEV.open()

    Menu {
      id: statsTglBtnMenuDV

      MenuItem { text: "Max DVs";  onTriggered: boxData.maxDVs(); enabled: !boxData.isMaxDVs }
      MenuItem { text: "Re-Roll DVs"; onTriggered: boxData.reRollDVs() }
      MenuItem { text: "Reset DVs"; onTriggered: boxData.resetDVs(); enabled: !boxData.isMinDVs }

      MenuSeparator {}
      MenuItem { text: "Close" }
    }

    Menu {
      id: statsTglBtnMenuEV

      MenuItem { text: "Max EVs"; onTriggered: boxData.maxEVs(); enabled: !boxData.isMaxEVs }
      MenuItem { text: "Re-Roll EVs"; onTriggered: boxData.reRollEVs() }
      MenuItem { text: "Reset EVs"; onTriggered: boxData.resetEVs(); enabled: !boxData.isMinEvs }

      MenuSeparator {}
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

        MainToolTip {
          text: "Shiny didn't exist in Gen 1, but Gen 2 provided a way to determine a shiny from Gen 1 data. This way has been kept up with ever since."
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
