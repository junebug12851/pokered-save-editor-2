import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14
import QtGraphicalEffects 1.14

import "../../fragments/header"
import "../../fragments/general"
import "../../fragments/screens/home"

Page {
  id: page

  Material.background: brg.settings.textColorLight

  Rectangle {
    anchors.left: dexShadow.right
    anchors.top: parent.top
    anchors.bottom: dexContainer.top
    anchors.right: parent.right

    color: Qt.darker(brg.settings.accentColor, 1.50)

    RowLayout {
      anchors.centerIn: parent
      spacing: 35

      RowLayout {
        spacing: 8

        Image {
          width: 20
          height: 20
          source: "qrc:/assets/icons/poke-go/pokeball.svg"
          fillMode: Image.PreserveAspectFit
          sourceSize.height: height
          sourceSize.width: width
        }

        Text {
          text: brg.file.data.dataExpanded.player.pokedex.ownedCount
          Layout.alignment: Qt.AlignVCenter

          font.pixelSize: 18
          color: brg.settings.textColorLight
        }
      }

      RowLayout {
        spacing: 8

        Image {
          id: totalSeenIcon
          width: 20
          height: 20
          opacity: 0.55
          source: "qrc:/assets/icons/poke-go/pokeball.svg"
          fillMode: Image.PreserveAspectFit
          sourceSize.height: height
          sourceSize.width: width

          Colorize {
            opacity: 0.55

            anchors.fill: totalSeenIcon
            source: totalSeenIcon
            hue: 0.0
            saturation: -1.0
            lightness: -0.30
          }
        }

        Text {
          text: brg.file.data.dataExpanded.player.pokedex.seenCount
          Layout.alignment: Qt.AlignVCenter

          font.pixelSize: 18
          color: brg.settings.textColorLight
        }
      }
    }
  }

  Rectangle {
    id: dexContainer
    width: Math.trunc(parent.width * 0.55)
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.topMargin: 50
    anchors.right: parent.right
    color: Qt.lighter(brg.settings.accentColor, 1.65)

    ListView {
      anchors.fill: parent
      clip: true
      model: brg.pokedexModel
      ScrollBar.vertical: ScrollBar {}
      leftMargin: 10

      delegate: Rectangle {
        function getMonUrl() {

          if(dexState === 0)
            return "qrc:/assets/icons/fontawesome/question.svg";

          var num = (dexInd+1).toString().padStart(3, "0");

          var name = dexName.toLowerCase();
          if(name === "nidoran<f>")
            name = "nidoran-f";
          else if(name === "nidoran<m>")
            name = "nidoran-m";
          else if(name === "mr.mime")
            name = "mrmime";

          return "qrc:/assets/icons/mon-icons/" + num + "-" + name + ".svg";
        }

        function fixName() {
          var n = dexName;

          if(n === "Nidoran<f>")
            n = "Nidoran ♀";
          else if(n === "Nidoran<m>")
            n = "Nidoran ♂";
          else if(n === "Mr.Mime")
            n = "Mr. Mime";

          return n;
        }

        function fixNum() {
          return (dexInd+1).toString().padStart(3, "0");
        }

        function getIconOpacity() {
          if(dexState === 0)
            return 0.25
          else if(dexState === 1)
            return 0.70
          else
            return 1.00
        }

        color: "transparent"
        width: parent.width
        height: 60

        RoundButton {

          function updatePreview() {
            if(hovered)
              previewImage.source = getMonUrl();
            else
              previewImage.source = "";

            if(dexState === 0)
              previewImage.opacity = 0.25;
            else
              previewImage.opacity = 1.00;
          }

          flat: true
          anchors.verticalCenter: parent.verticalCenter
          width: parent.width
          hoverEnabled: true

          onClicked: {
            brg.file.data.dataExpanded.player.pokedex.toggleOne(index)
            updatePreview();
          }

          onHoveredChanged: updatePreview();

          Image {
            id: icon
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            sourceSize.height: height
            sourceSize.width: width

            width: 40
            height: 40
            opacity: getIconOpacity()
            source: getMonUrl()
            fillMode: Image.PreserveAspectFit
          }

          Colorize {
            visible: (dexState === 2)
                     ? false
                     : true
            opacity: getIconOpacity()
            anchors.fill: icon
            source: icon
            hue: 0.0
            saturation: -0.6
            lightness: -0.85
          }

          Text {
            anchors.left: icon.right
            anchors.leftMargin: 15
            anchors.verticalCenter: parent.verticalCenter

            text: "No." + fixNum()
            font.pixelSize: 16
          }

          Text {
            anchors.centerIn: parent

            horizontalAlignment: Text.AlignLeft
            //width: font.pixelSize * 10

            text: (dexState === 0)
                  ? "??????"
                  : fixName()
            font.pixelSize: 16
          }

          Image {
            visible: (dexState === 0)
                     ? false
                     : true

            id: ownedIcon
            anchors.right: parent.right
            anchors.rightMargin: 50
            anchors.verticalCenter: parent.verticalCenter

            width: 20
            height: 20
            opacity: (dexState === 2)
                     ? 1.00
                     : 0.25

            source: "qrc:/assets/icons/poke-go/pokeball.svg"
            fillMode: Image.PreserveAspectFit
          }

          Colorize {
            visible: (dexState === 1)
                     ? true
                     : false

            opacity: (dexState === 2)
                     ? 1.00
                     : 0.25

            anchors.fill: ownedIcon
            source: ownedIcon
            hue: 0.0
            saturation: -1.0
            lightness: -0.50
          }
        }
      }
    }
  }

  Rectangle {
    id: dexShadow
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.right: dexContainer.left

    width: brg.settings.headerShadowHeight
    color: Qt.darker(brg.settings.accentColor, 1.15)
  }

  Image {
    id: previewImage

    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: dexShadow.left

    anchors.margins: 50

    fillMode: Image.PreserveAspectFit

    sourceSize.height: height
    sourceSize.width: width
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn2 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: brg.file.data.dataExpanded.player.pokedex.randomize();

    icon2.source: "qrc:/assets/icons/fontawesome/check-double.svg"
    text2: "Toggle All"
    onBtn2Clicked: brg.file.data.dataExpanded.player.pokedex.toggleAll();
  }
}
