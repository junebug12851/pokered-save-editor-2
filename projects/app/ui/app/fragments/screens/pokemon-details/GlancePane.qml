// GlancePane.qml -- the live at-a-glance summary (right half of PokemonDetails).
//
// A header bar with the species picker (SelectSpecies), an editable level field,
// and the status picker (SelectStatus); the mon's sprite (getMonUrl, refreshed on
// dv/shiny/species change); a StatsGroup (or StatsGroupInvalid for glitch mons
// with a party fallback); and a bottom HP slider with color-coded fill. Edits
// write straight to boxData (species/level/status/hp) and call the matching
// manual*Changed() so dependent data recomputes. The header combos override
// hoverColor because they sit on the accent bar. The (dexNum+1) padding is the
// correct 0->1 dex conversion -- do not change it.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

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
      hoverColor: brg.settings.textColorLight  // on the accent header bar
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
        function onSpeciesChanged() {
          brg.moveSelectModel.monFromBox(boxData);
          speciesSelect.currentIndex = brg.speciesSelectModel.speciesToListIndex(boxData.species);
        }
      }

      MainToolTip {
        text: qsTr("The Pokémon's species, glitch Pokémon are available.")
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

      text: qsTr("Lv.")
    }

    DefTextEdit {
      id: levelEdit

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      anchors.verticalCenter: parent.verticalCenter
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
        if(isNaN(idDec))
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
        function onLevelChanged() { levelEdit.text = boxData.level.toString(10); }
      }

      MainToolTip {
        text: qsTr("Pokémon level from 1 to 100, pokemon below level 5 could cause underflow bugs as per gen 1 game issues.")
      }
    }

    SelectStatus {
      id: statusSelect
      hoverColor: brg.settings.textColorLight  // on the accent header bar
      anchors.top: parent.top
      anchors.left: levelEdit.right
      anchors.leftMargin: 5
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 2

      onActivated: { boxData.status = currentValue; }
      Component.onCompleted: currentIndex = brg.statusSelectModel.statusToListIndex(boxData.status);

      Connections {
        target: boxData
        function onStatusChanged() { statusSelect.currentIndex = brg.statusSelectModel.statusToListIndex(boxData.status); }
      }

      MainToolTip {
        text: qsTr("The Pokémon's status, sleep also has number of turns left.")
      }
    }
  }

  Rectangle {
    id: statsData
    color: "transparent"

    // Size to the visible stats grid so the sprite anchors to its right instead of
    // overlapping it (the rect had no width before; the narrower pane exposed it).
    width: statsDataValid.visible ? statsDataValid.implicitWidth
                                  : statsDataInvalid.implicitWidth

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

      function onDvChanged() { monImg.source = monImg.getMonUrl(); }
      function onIsShinyChanged() { monImg.source = monImg.getMonUrl(); }
      function onSpeciesChanged() { monImg.source = monImg.getMonUrl(); }
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

      text: qsTr("HP")
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

      anchors.verticalCenter: parent.verticalCenter
      anchors.left: hpTxt.right
      anchors.leftMargin: 10
      anchors.right: parent.right
      anchors.rightMargin: 10

      from: 0
      to: getTo()

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
        color: hpEdit.getColor()

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
        visible: hpEdit.pressed || hpEdit.hovered
        text: hpEdit.value.toFixed(0)

        Material.background: brg.settings.textColorLight
        Material.foreground: brg.settings.textColorDark

        font.pixelSize: 14
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
      }

      onMoved: { boxData.hp = value; }
      Component.onCompleted: value = boxData.hp;

      Connections {
        target: boxData
        function onHpChanged() { hpEdit.value = boxData.hp; }
      }

      MainToolTip {
        text: qsTr("HP Left from the Pokémon's maximum")
      }
    }
  }
}
