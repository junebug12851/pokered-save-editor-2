import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.ItemStorageBox 1.0
import App.ItemStorageModel 1.0

import "../../header"
import "../../general"
import "../../screens/home"

Rectangle {
  id: top
  property string title: ""
  property ItemStorageModel model: null
  property ItemStorageBox box: null

  Rectangle {
    id: bagHeader
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.right: parent.right
    height: 45

    Material.foreground: brg.settings.textColorLight
    Material.background: brg.settings.accentColor
    color: brg.settings.accentColor

    RowLayout {
      anchors.centerIn: parent
      spacing: 15

      IconButtonSquare {
        icon.source: "qrc:/assets/icons/fontawesome/check-double.svg"
        onClicked: model.checkedToggleAll()

        rightInset: 0
        leftInset: 0

        leftPadding: 0
        rightPadding: 0
      }

      Text {
        text: title
        font.pixelSize: 18
        color: brg.settings.textColorLight
        rightPadding: 0
      }

      IconButtonSquare {
        icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
        onClicked: box.reset();

        rightInset: 0
        leftInset: 0

        leftPadding: 0
        rightPadding: 0
      }

      IconButtonSquare {
        //enabled: !box.relocateFull // Works only sometimes for some reason
        icon.source: "qrc:/assets/icons/fontawesome/exchange-alt.svg"
        onClicked: box.relocateAll();

        rightInset: 0
        leftInset: 0

        leftPadding: 0
        rightPadding: 0
      }
    }
  }

  ItemBoxView {
    id: bagView

    model: top.model

    anchors.top: bagHeader.bottom
    anchors.left: parent.left
    anchors.leftMargin: 15
    anchors.right: parent.right
    anchors.rightMargin: 15
    anchors.bottom: bagFooter.top

    box: top.box
  }

  Rectangle {
    id: bagFooter
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

      Text {
        visible: !model.hasChecked
        text: "" + box.itemsCount
              + " / "
              + box.itemsMax
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }

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

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
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
