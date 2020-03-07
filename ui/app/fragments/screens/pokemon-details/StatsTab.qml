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
}
