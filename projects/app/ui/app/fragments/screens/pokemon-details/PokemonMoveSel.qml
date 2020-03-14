import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0
import App.PokemonMove 1.0

import "../../general"
import "../../header"
import "../../controls/selection"

Rectangle {
  id: top
  property PokemonMove monMove: null
  radius: 15
  implicitHeight: topContents.implicitHeight
  implicitWidth: topContents.implicitWidth
  color: (monMove.moveID === "phony")
         ? getColor()
         : getColor()

  // Thanks to Bulbapedia
  // bulbapedia.bulbagarden.net/wiki/Category:Type_color_templates
  // Licensed: CC-BY-NC-SA 2.5 (creativecommons.org/licenses/by-nc-sa/2.5)
  function getColor() {
    if(monMove.moveType === "Normal")
      return "#A8A878";
    else if(monMove.moveType === "Fighting")
      return Qt.lighter("#C03028", 1.50);
    else if(monMove.moveType === "Flying")
      return "#A890F0";
    else if(monMove.moveType === "Poison")
      return Qt.lighter("#A040A0", 1.50);
    else if(monMove.moveType === "Ground")
      return "#E0C068";
    else if(monMove.moveType === "Rock")
      return "#B8A038";
    else if(monMove.moveType === "Bug")
      return "#A8B820";
    else if(monMove.moveType === "Ghost")
      return Qt.lighter("#705898", 1.50);
    else if(monMove.moveType === "Fire")
      return "#F08030";
    else if(monMove.moveType === "Water")
      return "#6890F0";
    else if(monMove.moveType === "Grass")
      return "#78C850";
    else if(monMove.moveType === "Electric")
      return "#F8D030";
    else if(monMove.moveType === "Psychic")
      return "#F85888";
    else if(monMove.moveType === "Ice")
      return "#98D8D8";
    else if(monMove.moveType === "Dragon")
      return Qt.lighter("#7038F8", 1.50);

    // Used TCG Colorless for glitch color
    return "#E5D6D0";
  }

  RowLayout {
    id: topContents
    width: parent.width
    spacing: 5

    SelectMove {
      id: moveSelect

      onActivated: monMove.moveID = currentValue;
      Component.onCompleted: currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);

      Connections {
        target: monMove
        onMoveIDChanged: moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
      }

      Connections {
        target: brg.moveSelectModel
        onMonChanged: {
          moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
        }
      }

      Connections {
        target: boxData
        onSpeciesChanged: {
          moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
        }
      }

      MainToolTip {
        text: "Pokemon Move"
      }
    }

    Text {
      visible: monMove.moveID !== 0

      Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
      horizontalAlignment: Text.AlignHCenter
      Layout.leftMargin: -15

      Layout.preferredWidth: font.pixelSize * 5
      color: brg.settings.textColorDark

      text: monMove.moveType
      font.pixelSize: 13
      font.capitalization: Font.Capitalize
    }

    DefTextEdit {
      id: movePPEdit
      visible: monMove.moveID !== 0

      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter

      width: font.pixelSize * 2
      implicitWidth: font.pixelSize * 2
      maximumLength: 3
      color: brg.settings.textColorDark

      Layout.topMargin: 16

      onTextChanged: {
        if(text === "")
          return;

        var idDec = parseInt(text, 10);
        if(idDec === NaN)
          return;

        if(idDec < 0 || idDec > 0xFF)
          return;

        monMove.pp = idDec;
      }
      Component.onCompleted: text = monMove.pp.toString(10);

      Connections {
        target: monMove
        onPpChanged: movePPEdit.text = monMove.pp.toString(10);
      }

      MainToolTip {
        text: "Pokemon PP"
      }
    }

    Text {
      visible: monMove.moveID !== 0

      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorDark

      text: "/"
      font.pixelSize: 13
      font.capitalization: Font.Capitalize
    }

    Text {
      visible: monMove.moveID !== 0

      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorDark

      text: monMove.getMaxPP
      font.pixelSize: 13
      font.capitalization: Font.Capitalize
    }

    IconButtonSquare {
      visible: monMove.moveID !== 0

      icon.width: 7

      icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
      icon.color: brg.settings.textColorDark

      onClicked: moveSelMenu.open();

      Menu {
        id: moveSelMenu

        Menu {
          title: "PP Up"

          MenuItem { text: "Currently: " + monMove.ppUp + "/3"}
          MenuSeparator { }
          MenuItem { text: "Max Out"; onTriggered: monMove.maxPpUp(); enabled: monMove.ppUp < 3 }
          MenuItem { text: "Raise"; onTriggered: monMove.raisePpUp(); enabled: monMove.ppUp < 3 }
          MenuItem { text: "Lower"; onTriggered: monMove.lowerPpUp(); enabled: monMove.ppUp > 0 }
          MenuItem { text: "Reset"; onTriggered: monMove.resetPpUp(); enabled: monMove.ppUp > 0 }
        }

        Menu {
          title: "All Moves"

          MenuItem { text: "Clear All"; onTriggered: boxData.clearMoves(); }
          MenuItem { text: "Re-Roll All"; onTriggered: boxData.randomizeMoves(); }
          MenuItem { text: "Correct All"; onTriggered: boxData.correctMoves(); }
          MenuItem { text: "Move Empty"; onTriggered: boxData.cleanupMoves(); }
          MenuItem { text: "Correct / Move"; onTriggered: {
              boxData.correctMoves();
              boxData.cleanupMoves();
            }
          }
        }

        MenuItem { text: "Restore PP"; onTriggered: monMove.restorePP(); enabled: !monMove.isMaxPP }
        MenuItem { text: "Re-Roll"; onTriggered: monMove.randomize(); }
        MenuItem { text: "Corect Move"; onTriggered: monMove.correctMove(); }

        MenuSeparator { }
        MenuItem { text: "Close" }
      }
    }
  }
}
