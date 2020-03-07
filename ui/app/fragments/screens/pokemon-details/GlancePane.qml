import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "./stats"
import "../../general"
import "../../header"
import "../../controls/selection"

Rectangle {
  id: top
  color: "transparent"

  property PokemonBox boxData: null
  property PokemonParty partyData: null

  Rectangle {
    id: topBar
    Material.background: brg.settings.accentColor
    Material.foreground: brg.settings.textColorLight
    color: brg.settings.accentColor

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 45

    SelectSpecies {
      id: speciesSelect
      anchors.top: parent.top
      anchors.left: parent.left
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 2

      width: font.pixelSize * 14

      onActivated: {
        if(currentValue === boxData.species)
          return;

        var hasNickname = boxData.hasNickname;
        boxData.species = currentValue;
        boxData.manualSpeciesChanged();
        if(!hasNickname)
          boxData.changeName(true);
      }
      Component.onCompleted: currentIndex = brg.speciesSelectModel.speciesToListIndex(boxData.species);

      Connections {
        target: boxData
        onSpeciesChanged: speciesSelect.currentIndex = brg.speciesSelectModel.speciesToListIndex(boxData.species);
      }
    }

    Text {
      id: levelTxt

      anchors.top: parent.top
      anchors.left: speciesSelect.right
      anchors.leftMargin: 5
      anchors.bottom: parent.bottom

      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignRight
      color: brg.settings.textColorLight

      font.pixelSize: 14

      text: "Lv."
    }

    DefTextEdit {
      id: levelEdit

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      anchors.top: parent.top
      anchors.topMargin: 14
      anchors.left: levelTxt.right
      anchors.leftMargin: 5

      width: font.pixelSize * 3
      color: brg.settings.textColorLight
      maximumLength: 3

      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignLeft

      onTextChanged: {
        if(text === "")
          return;

        var idDec = parseInt(text, 10);
        if(idDec === NaN)
          return;

        if(idDec < 0 || idDec > 100)
          return;

        if(idDec === boxData.level)
          return;

        boxData.level = idDec;
        boxData.manualLevelChanged();
      }
      Component.onCompleted: text = boxData.level.toString(10);

      Connections {
        target: boxData
        onLevelChanged: levelEdit.text = boxData.level.toString(10);
      }
    }

    SelectStatus {
      id: statusSelect
      anchors.top: parent.top
      anchors.left: levelEdit.right
      anchors.leftMargin: 5
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 2

      onActivated: boxData.status = currentValue;
      Component.onCompleted: currentIndex = brg.statusSelectModel.statusToListIndex(boxData.status);

      Connections {
        target: boxData
        onStatusChanged: statusSelect.currentIndex = brg.statusSelectModel.statusToListIndex(boxData.status);
      }
    }
  }

  Rectangle {
    id: statsData
//    width: (statsDataValid.visible)
//           ? statsDataValid.implicitWidth
//           : statsDataInvalid.implicitWidth

    anchors.top: topBar.bottom
    anchors.topMargin: 15
    anchors.left: parent.left
    anchors.leftMargin: 0
    anchors.bottom: bottomBar.top
    anchors.bottomMargin: 5

    StatsGroup {
      id: statsDataValid
      visible: boxData.isValidBool ||
               (!boxData.isValidBool && partyData === null)
      anchors.fill: parent
    }

    StatsGroupInvalid {
      id: statsDataInvalid
      partyData: top.partyData

      visible: !boxData.isValidBool && partyData !== null
      anchors.fill: parent
    }
  }

  Image {
    id: monImg
    anchors.top: topBar.bottom
    anchors.left: statsData.right
    anchors.right: parent.right
    anchors.bottom: bottomBar.top
    anchors.margins: 15

    sourceSize.height: height
    sourceSize.width: width

    Component.onCompleted: source = getMonUrl();

    Connections {
      target: boxData

      onDvChanged: monImg.source = monImg.getMonUrl();
      onIsShinyChanged: monImg.source = monImg.getMonUrl();
      onSpeciesChanged: monImg.source = monImg.getMonUrl();
    }

    fillMode: Image.PreserveAspectFit

    function getMonUrl() {

      if(!boxData.isValidBool)
        return "qrc:/assets/icons/fontawesome/question.svg";

      var num = (boxData.dexNum+1).toString().padStart(3, "0");

      var name = boxData.speciesName.toLowerCase();
      if(name === "nidoran<f>")
        name = "nidoran-f";
      else if(name === "nidoran<m>")
        name = "nidoran-m";
      else if(name === "mr.mime")
        name = "mrmime";

      var shiny = (boxData.isShiny)
          ? "-shiny"
          : ""

      return "qrc:/assets/icons/mon-icons/" + num + "-" + name + shiny + ".svg";
    }
  }

  Rectangle {
    id: bottomBar
    Material.background: brg.settings.accentColor
    Material.foreground: brg.settings.textColorLight
    color: brg.settings.accentColor

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    height: 45

    Text {
      id: hpTxt

      anchors.top: parent.top
      anchors.left: parent.left
      anchors.leftMargin: 5
      anchors.bottom: parent.bottom

      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignRight
      color: brg.settings.textColorLight

      font.pixelSize: 14

      text: "HP"
    }

    Slider {
      id: hpEdit

      function getTo() {
        if(boxData.isValidBool)
          return boxData.hpStat
        else if(!boxData.isValidBool && partyData !== null)
          return partyData.maxHP;

        return 0xFFFF;
      }

      anchors.top: parent.top
      anchors.topMargin: 8
      anchors.left: hpTxt.right
      anchors.leftMargin: 10
      anchors.right: parent.right
      anchors.rightMargin: 10

      from: 0
      to: (boxData.isValidBool || boxData.hpStat === "phony")
          ? getTo()
          : getTo()

      Material.foreground: brg.settings.textColorDark
      Material.background: brg.settings.textColorLight

      function getColor() {
        var pos = hpEdit.position;

        // Green
        if(pos > 0.5)
          return "#4CAF50";

        // Amber
        else if(pos > 0.2)
          return "#FFA000";

        // Red
        else
          return "#D32F2F";
      }

      background: Rectangle {
        x: hpEdit.leftPadding
        y: hpEdit.topPadding + hpEdit.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 4
        width: hpEdit.availableWidth
        height: implicitHeight
        radius: 2
        color: hpEdit.value === "phony"
               ? hpEdit.getColor()
               : hpEdit.getColor()

        Rectangle {
          width: hpEdit.visualPosition * parent.width
          height: parent.height
          color: brg.settings.textColorLight
          radius: 2
        }
      }

      handle: Rectangle {
        x: hpEdit.leftPadding + hpEdit.visualPosition * (hpEdit.availableWidth - width)
        y: hpEdit.topPadding + hpEdit.availableHeight / 2 - height / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 13
        color: hpEdit.pressed
               ? Qt.darker(brg.settings.textColorLight, 1.25)
               : brg.settings.textColorLight
        border.color: brg.settings.textColorMid
      }

      ToolTip {
        parent: hpEdit.handle
        visible: hpEdit.pressed
        text: hpEdit.value.toFixed(0)

        Material.background: brg.settings.textColorLight
        Material.foreground: brg.settings.textColorDark

        font.pixelSize: 14
      }

      onMoved: boxData.hp = value;
      Component.onCompleted: value = boxData.hp;

      Connections {
        target: boxData
        onHpChanged: hpEdit.value = boxData.hp;
      }
    }
  }
}
