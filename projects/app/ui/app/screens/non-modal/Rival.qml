import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top

  Rectangle {
    anchors.centerIn: parent

    color: "transparent"
    width: 1
    height: 1

    NameDisplay {
      id: rivalNameEdit
      anchors.top: parent.top
      anchors.topMargin: -110
//      anchors.left: parent.left
      anchors.horizontalCenter: parent.horizontalCenter

      isPersonName: true
      isPlayerName: false

      onStrChanged: {
        let r = brg.file.data.dataExpanded.rival;
        if(r) r.name = str;
      }

      Connections {
        target: brg.file.data.dataExpanded ? brg.file.data.dataExpanded.rival : null
        function onNameChanged() {
          let r = brg.file.data.dataExpanded.rival;
          if(r) rivalNameEdit.str = r.name;
        }
      }

      Component.onCompleted: {
        let r = brg.file.data.dataExpanded ? brg.file.data.dataExpanded.rival : null;
        if(r) rivalNameEdit.str = r.name;
      }
    }

    Image {
      source: "qrc:/assets/images/rival-larger.png"

      //anchors.left: parent.left
      anchors.top: rivalNameEdit.bottom
      anchors.topMargin: 20
      anchors.horizontalCenter: parent.horizontalCenter

      anchors.bottom: rivalStarterEdit.top
      anchors.bottomMargin: 10

      width: top.width / 3

      fillMode: Image.PreserveAspectFit
    }

    ComboBox {
      id: rivalStarterEdit
      anchors.bottom: parent.bottom
      anchors.bottomMargin: -125
      //anchors.left: parent.left
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.horizontalCenterOffset: 40

      textRole: "monName"
      valueRole: "monInd"

      font.capitalization: Font.Capitalize
      font.pixelSize: 14
      flat: true
      model: brg.starterModel
      width: font.pixelSize * 10

      // Borderless: clean look, no frame around the combo (Twilight's call s13n).
      background: Rectangle { color: "transparent"; border.width: 0 }

      onActivated: {
        let r = brg.file.data.dataExpanded ? brg.file.data.dataExpanded.rival : null;
        if(r) r.starter = currentValue;
      }
      Component.onCompleted: {
        let r = brg.file.data.dataExpanded ? brg.file.data.dataExpanded.rival : null;
        if(r) currentIndex = brg.starterModel.valToIndex(r.starter);
      }

      MainToolTip {
        text: "Set at start of game and determines your rivals team, namely which Pokemon he has growing with him."
      }

      Connections {
        target: brg.file.data.dataExpanded ? brg.file.data.dataExpanded.rival : null
        function onStarterChanged() {
          let r = brg.file.data.dataExpanded ? brg.file.data.dataExpanded.rival : null;
          if(r) rivalStarterEdit.currentIndex = brg.starterModel.valToIndex(r.starter);
        }
      }

      Label {
        anchors.right: parent.left
        anchors.rightMargin: 7
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        font.pixelSize: 14
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text: "Starter"
      }
    }
  }

  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      let r = brg.file.data.dataExpanded ? brg.file.data.dataExpanded.rival : null;
      if(r) r.randomize();
    }
  }
}

