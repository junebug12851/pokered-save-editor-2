import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

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

      onStrChanged: brg.file.data.dataExpanded.rival.name = str;

      Connections {
        target: brg.file.data.dataExpanded.rival
        onNameChanged: rivalNameEdit.str = brg.file.data.dataExpanded.rival.name;
      }

      Component.onCompleted: rivalNameEdit.str = brg.file.data.dataExpanded.rival.name;
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

      onActivated: brg.file.data.dataExpanded.rival.starter = currentValue;
      Component.onCompleted: currentIndex = brg.starterModel.valToIndex(brg.file.data.dataExpanded.rival.starter);

      MainToolTip {
        text: "Set at start of game and determines your rivals team, namely which Pokemon he has growing with him."
      }

      Connections {
        target: brg.file.data.dataExpanded.rival
        onStarterChanged: rivalStarterEdit.currentIndex = brg.starterModel.valToIndex(brg.file.data.dataExpanded.rival.starter);
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

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      brg.file.data.dataExpanded.rival.randomize();
    }
  }
}
