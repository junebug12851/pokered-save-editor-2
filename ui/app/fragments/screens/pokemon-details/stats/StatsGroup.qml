import QtQuick 2.14
import QtCharts 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../../general"
import "../../../header"

Rectangle {
  Text {
    id: hpStatTxt
    anchors.top: parent.top
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "HP"
  }

  Text {
    id: hpStatNum
    anchors.top: hpStatTxt.top
    anchors.left: hpStatTxt.right
    anchors.leftMargin: 5
    anchors.bottom: hpStatTxt.bottom

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14

    text: (boxData.isValidBool)
          ? boxData.hpStat
          : "???"
  }

  Text {
    id: atkStatTxt
    anchors.top: hpStatTxt.bottom
    anchors.topMargin: 5
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Atk"
  }

  Text {
    id: atkStatNum
    anchors.top: atkStatTxt.top
    anchors.left: atkStatTxt.right
    anchors.leftMargin: 5
    anchors.bottom: atkStatTxt.bottom

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: (boxData.isValidBool)
          ? boxData.atkStat
          : "???"
  }

  Text {
    id: defStatTxt
    anchors.top: atkStatTxt.bottom
    anchors.topMargin: 5
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Def"
  }

  Text {
    id: defStatNum
    anchors.top: defStatTxt.top
    anchors.left: defStatTxt.right
    anchors.leftMargin: 5
    anchors.bottom: defStatTxt.bottom

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: (boxData.isValidBool)
          ? boxData.defStat
          : "???"
  }

  Text {
    id: spdStatTxt
    anchors.top: defStatTxt.bottom
    anchors.topMargin: 5
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Spd"
  }

  Text {
    id: spdStatNum
    anchors.top: spdStatTxt.top
    anchors.left: spdStatTxt.right
    anchors.leftMargin: 5
    anchors.bottom: spdStatTxt.bottom

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: (boxData.isValidBool)
          ? boxData.spdStat
          : "???"
  }

  Text {
    id: spStatTxt
    anchors.top: spdStatTxt.bottom
    anchors.topMargin: 5
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Sp"
  }

  Text {
    id: spStatNum
    anchors.top: spStatTxt.top
    anchors.left: spStatTxt.right
    anchors.leftMargin: 5
    anchors.bottom: spStatTxt.bottom

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: (boxData.isValidBool)
          ? boxData.spStat
          : "???"
  }
}
