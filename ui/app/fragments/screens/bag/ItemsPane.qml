import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.ItemStorageBox 1.0

import "../../header"
import "../../general"
import "../../screens/home"

Rectangle {
  id: top
  property string title: ""
  property alias model: bagView.model
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

      Text {
        text: title
        font.pixelSize: 18
        color: brg.settings.textColorLight
      }

      IconButtonSquare {
        icon.source: "qrc:/assets/icons/fontawesome/broom.svg"
        onClicked: box.reset();
      }
    }
  }

  ItemBoxView {
    id: bagView

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

    RowLayout {
      anchors.centerIn: parent
      spacing: 15

      Text {
        text: "" + box.itemsCount
              + " / "
              + box.itemsMax
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }
    }
  }
}
