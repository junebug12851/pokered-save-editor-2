import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../general"
import "../../header"
import "../../controls/selection"

Rectangle {
  id: top
  color: "transparent"

  property PokemonBox boxData: null

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

  Image {
    id: monImg
    anchors.top: topBar.bottom
    anchors.left: parent.left
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
  }
}
