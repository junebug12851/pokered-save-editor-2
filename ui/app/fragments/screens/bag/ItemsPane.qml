import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import app.itemstoragebox 1.0

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
    color: brg.settings.accentColor
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.right: parent.right
    height: 30

    RowLayout {
      anchors.centerIn: parent
      spacing: 15

      Text {
        text: title
        font.pixelSize: 18
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
    height: 30

    RowLayout {
      anchors.centerIn: parent
      spacing: 15

      Text {
        text: "" + box.itemsCount
              + " / "
              + box.itemsMax
        font.pixelSize: 14
      }

      Text {
        text: "$" + box.itemsWorth
        font.pixelSize: 14
      }
    }
  }
}
