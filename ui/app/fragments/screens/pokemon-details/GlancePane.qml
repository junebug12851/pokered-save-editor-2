import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../general"
import "../../header"

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
