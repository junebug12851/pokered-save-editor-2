import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../../general"
import "../../../header"

// StatsGroupInvalid.qml -- the editable raw-stats grid for glitch/invalid party
// mons.
//
// Editable raw stats for an "invalid" box mon that has live party data
// (max HP / Atk / Def / Spd / Sp). Clean label | field grid — no per-row
// anchor offsets.
Rectangle {
  property PokemonParty partyData: null

  Material.foreground: brg.settings.textColorDark
  Material.background: brg.settings.textColorLight

  GridLayout {
    anchors.top: parent.top
    anchors.left: parent.left

    columns: 2
    columnSpacing: 5
    rowSpacing: 3

    // ---- HP ----
    Text {
      text: qsTr("HP")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    DefTextEdit {
      id: hpStatNum
      maximumLength: 5
      Layout.preferredWidth: font.pixelSize * 5
      Layout.alignment: Qt.AlignVCenter
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignLeft

      onTextChanged: {
        if(text === "")
          return;
        var idDec = parseInt(text, 10);
        if(isNaN(idDec))
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
        target: (partyData === null) ? null : partyData
        function onMaxHPChanged() { hpStatNum.text = partyData.maxHP.toString(10); }
      }
    }

    // ---- Atk ----
    Text {
      text: qsTr("Atk")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    DefTextEdit {
      id: atkStatNum
      maximumLength: 5
      Layout.preferredWidth: font.pixelSize * 5
      Layout.alignment: Qt.AlignVCenter
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignLeft

      onTextChanged: {
        if(text === "")
          return;
        var idDec = parseInt(text, 10);
        if(isNaN(idDec))
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
        target: (partyData === null) ? null : partyData
        function onAttackChanged() { atkStatNum.text = partyData.attack.toString(10); }
      }
    }

    // ---- Def ----
    Text {
      text: qsTr("Def")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    DefTextEdit {
      id: defStatNum
      maximumLength: 5
      Layout.preferredWidth: font.pixelSize * 5
      Layout.alignment: Qt.AlignVCenter
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignLeft

      onTextChanged: {
        if(text === "")
          return;
        var idDec = parseInt(text, 10);
        if(isNaN(idDec))
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
        target: (partyData === null) ? null : partyData
        function onDefenseChanged() { defStatNum.text = partyData.defense.toString(10); }
      }
    }

    // ---- Spd ----
    Text {
      text: qsTr("Spd")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    DefTextEdit {
      id: spdStatNum
      maximumLength: 5
      Layout.preferredWidth: font.pixelSize * 5
      Layout.alignment: Qt.AlignVCenter
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignLeft

      onTextChanged: {
        if(text === "")
          return;
        var idDec = parseInt(text, 10);
        if(isNaN(idDec))
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
        target: (partyData === null) ? null : partyData
        function onSpeedChanged() { spdStatNum.text = partyData.speed.toString(10); }
      }
    }

    // ---- Sp ----
    Text {
      text: qsTr("Sp")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    DefTextEdit {
      id: spStatNum
      maximumLength: 5
      Layout.preferredWidth: font.pixelSize * 5
      Layout.alignment: Qt.AlignVCenter
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignLeft

      onTextChanged: {
        if(text === "")
          return;
        var idDec = parseInt(text, 10);
        if(isNaN(idDec))
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
        target: (partyData === null) ? null : partyData
        function onSpecialChanged() { spStatNum.text = partyData.special.toString(10); }
      }
    }
  }
}
