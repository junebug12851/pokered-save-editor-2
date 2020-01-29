import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../fragments"
import "../common/Style.js" as Style

Rectangle {
  color: Style.primaryColorLight
  anchors.fill: parent

  Image {
    asynchronous: true
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent
    source: "qrc:/assets/wallpaper/starters.jpg"
    opacity: .35
  }

  ListView {
    id: creditsView
    clip: true
    model: creditsModel
    anchors.fill: parent

    delegate: ColumnLayout {

      property bool isLast: index+1 < creditsView.count ? false : true

      spacing: 0
      width: parent.width

      Text {
        visible: section !== ""
        text: section
        font.pixelSize: 20
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        color: Style.textColorPrimary
        topPadding: 30
      }

      Text {
        visible: creditTo !== ""
        text: creditTo
        font.pixelSize: 15
        Layout.alignment: Qt.AlignVCenter
        color: Style.textColorPrimary
        topPadding: 30
        leftPadding: 15
      }

      Text {
        visible: mandated !== ""
        text: "\" " + mandated + " \""
        font.pixelSize: 13
        Layout.alignment: Qt.AlignVCenter
        color: Style.textColorPrimary
        leftPadding: 35
        topPadding: 10
      }

      Text {
        visible: urlTo !== ""
        text: urlTo
        font.pixelSize: 13
        font.italic: true
        Layout.alignment: Qt.AlignVCenter
        color: Qt.lighter(Style.textColorPrimary, 1.25)
        leftPadding: 35
        topPadding: 10
      }

      Text {
        visible: license !== ""
        text: license
        font.pixelSize: 13
        font.italic: true
        Layout.alignment: Qt.AlignVCenter
        color: Qt.lighter(Style.textColorPrimary, 1.25)
        leftPadding: 35
        topPadding: 10
      }

      Text {
        visible: note !== ""
        text: note
        font.pixelSize: 14
        Layout.alignment: Qt.AlignVCenter
        color: Qt.darker(Style.textColorPrimary, 1.25)
        leftPadding: 35
        topPadding: 10
      }

      Text {
        visible: isLast
        bottomPadding: 75
      }
    }
  }

  ModalClose {
    onClicked: root.changeScreen("home")
  }

  CreditWork {
    text: "\"Basic Pokemons Colors\" by yoshiyaki (CC-BY-NC-ND 3.0)\n" +
          "https://www.deviantart.com/yoshiyaki/art/Basic-Pokemons-Colors-574585879"
  }
}
