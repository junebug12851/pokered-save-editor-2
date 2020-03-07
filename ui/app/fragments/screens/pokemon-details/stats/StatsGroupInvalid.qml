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
  property PokemonParty partyData: null

  Material.foreground: brg.settings.textColorDark
  Material.background: brg.settings.textColorLight

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

  DefTextEdit {
    id: hpStatNum
    anchors.top: hpStatTxt.top
    anchors.left: hpStatTxt.right
    anchors.leftMargin: 5

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    maximumLength: 5
    width: font.pixelSize * 5

    onTextChanged: {
      if(text === "")
        return;

      var idDec = parseInt(text, 10);
      if(idDec === NaN)
        return;

      if(idDec < 0 || idDec > 0xFFFF)
        return;

      partyData.maxHP = idDec;
    }
    Component.onCompleted: {
      if(partyData === null)
        return;

      text = partyData.maxHP.toString(10);
    }

    Connections {
      target: (partyData === null)
              ? null
              : partyData
      onMaxHPChanged: hpStatNum.text = partyData.maxHP.toString(10);
    }
  }

  Text {
    id: atkStatTxt
    anchors.top: hpStatTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Atk"
  }

  DefTextEdit {
    id: atkStatNum
    anchors.top: atkStatTxt.top
    anchors.left: atkStatTxt.right
    anchors.leftMargin: 5

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    maximumLength: 5
    width: font.pixelSize * 5

    onTextChanged: {
      if(text === "")
        return;

      var idDec = parseInt(text, 10);
      if(idDec === NaN)
        return;

      if(idDec < 0 || idDec > 0xFFFF)
        return;

      partyData.attack = idDec;
    }
    Component.onCompleted: {
      if(partyData === null)
        return;

      text = partyData.attack.toString(10);
    }

    Connections {
      target: (partyData === null)
              ? null
              : partyData

      onAttackChanged: atkStatNum.text = partyData.attack.toString(10);
    }
  }

  Text {
    id: defStatTxt
    anchors.top: atkStatTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Def"
  }

  DefTextEdit {
    id: defStatNum
    anchors.top: defStatTxt.top
    anchors.left: defStatTxt.right
    anchors.leftMargin: 5

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    maximumLength: 5
    width: font.pixelSize * 5

    onTextChanged: {
      if(text === "")
        return;

      var idDec = parseInt(text, 10);
      if(idDec === NaN)
        return;

      if(idDec < 0 || idDec > 0xFFFF)
        return;

      partyData.defense = idDec;
    }
    Component.onCompleted: {
      if(partyData === null)
        return;

      text = partyData.defense.toString(10);
    }

    Connections {
      target: (partyData === null)
              ? null
              : partyData
      onDefenseChanged: defStatNum.text = partyData.defense.toString(10);
    }
  }

  Text {
    id: spdStatTxt
    anchors.top: defStatTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Spd"
  }

  DefTextEdit {
    id: spdStatNum
    anchors.top: spdStatTxt.top
    anchors.left: spdStatTxt.right
    anchors.leftMargin: 5

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    maximumLength: 5
    width: font.pixelSize * 5

    onTextChanged: {
      if(text === "")
        return;

      var idDec = parseInt(text, 10);
      if(idDec === NaN)
        return;

      if(idDec < 0 || idDec > 0xFFFF)
        return;

      partyData.speed = idDec;
    }
    Component.onCompleted: {
      if(partyData === null)
        return;

      text = partyData.speed.toString(10);
    }

    Connections {
      target: (partyData === null)
              ? null
              : partyData

      onSpeedChanged: spdStatNum.text = partyData.speed.toString(10);
    }
  }

  Text {
    id: spStatTxt
    anchors.top: spdStatTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    font.bold: true

    text: "Sp"
  }

  DefTextEdit {
    id: spStatNum
    anchors.top: spStatTxt.top
    anchors.left: spStatTxt.right
    anchors.leftMargin: 5

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14
    maximumLength: 5
    width: font.pixelSize * 5

    onTextChanged: {
      if(text === "")
        return;

      var idDec = parseInt(text, 10);
      if(idDec === NaN)
        return;

      if(idDec < 0 || idDec > 0xFFFF)
        return;

      partyData.special = idDec;
    }
    Component.onCompleted: {
      if(partyData === null)
        return;

      text = partyData.special.toString(10);
    }

    Connections {
      target: (partyData === null)
              ? null
              : partyData

      onSpecialChanged: spStatNum.text = partyData.special.toString(10);
    }
  }
}
