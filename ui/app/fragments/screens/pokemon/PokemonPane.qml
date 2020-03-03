import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonStorageModel 1.0
import App.PokemonBoxSelectModel 1.0

import "../../header"
import "../../general"
import "../../controls/selection"
import "../../screens/home"

Rectangle {
  id: top
  property PokemonStorageModel model: null
  property PokemonBoxSelectModel selectModel: null

  Rectangle {
    id: boxHeader
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.right: parent.right
    height: 45

    Material.foreground: brg.settings.textColorLight
    Material.background: brg.settings.accentColor
    color: brg.settings.accentColor

    Row {
      anchors.centerIn: parent
      height: parent.height
      width: 265

      IconButtonSquare {
        icon.source: "qrc:/assets/icons/fontawesome/check-double.svg"
        onClicked: model.checkedToggleAll()

        rightInset: 0
        leftInset: 0
        leftPadding: 0
        rightPadding: 0
      }

      SelectPokemonBox {
        model: selectModel
        onActivated: top.model.switchBox(currentValue - 1);
        Component.onCompleted: currentIndex = top.model.curBox + 1;
      }

      IconButtonSquare {
        visible: (brg.file.data.dataExpanded.storage.curBox != model.curBox) &&
                 (model.curBox >= 0)

        icon.source: "qrc:/assets/icons/fontawesome/dot-circle.svg"
        onClicked: brg.file.data.dataExpanded.storage.curBox = model.curBox;

        rightInset: 0
        leftInset: 0

        leftPadding: 0
        rightPadding: 0
      }
    }
  }

  PokemonBoxView {
    id: pokemonView

    model: top.model

    anchors.top: boxHeader.bottom
    anchors.left: parent.left
    anchors.leftMargin: 15
    anchors.right: parent.right
    anchors.rightMargin: 15
    anchors.bottom: pokemonFooter.top

    theModel: top.model
  }

  Rectangle {
    id: pokemonFooter
    color: brg.settings.accentColor
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    height: 45

    Material.foreground: brg.settings.textColorLight
    Material.background: brg.settings.accentColor

    RowLayout {
      anchors.centerIn: parent
      spacing: 15

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-double-up.svg"
        onClicked: model.checkedMoveToTop();

        leftPadding: 0
        rightPadding: 0

        rightInset: 0
        leftInset: 0
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-up.svg"
        onClicked: model.checkedMoveUp();

        leftPadding: 0
        rightPadding: 0

        rightInset: 0
        leftInset: 0
      }

      // Releasing a pokemon probably shouldn't be represented with a trashcan
      // icon. just saying... The trash can icon shows clearest that we want to
      // remove the data.... but probably shouldn't forget that a living breathing
      // animal is represented with that icon and the implications it gives by
      // clicking the "Trash Can" to delete the Pokemon data.
      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/times.svg"
        onClicked: model.checkedDelete();

        leftPadding: 0
        rightPadding: 0

        rightInset: 0
        leftInset: 0
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/exchange-alt.svg"
        onClicked: model.checkedTransfer();

        leftPadding: 0
        rightPadding: 0

        rightInset: 0
        leftInset: 0
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-down.svg"
        onClicked: model.checkedMoveDown();

        leftPadding: 0
        rightPadding: 0

        rightInset: 0
        leftInset: 0
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-double-down.svg"
        onClicked: model.checkedMoveToBottom();

        leftPadding: 0
        rightPadding: 0

        rightInset: 0
        leftInset: 0
      }
    }
  }
}
