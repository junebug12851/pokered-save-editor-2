// ItemsPane.qml -- one titled item box (used twice on the Bag screen).
//
// Wraps an ItemBoxView between a colored header bar (a centered RowLayout of
// check-all toggle + title + live count/max) and a footer bar of bulk actions
// that appear only when items are checked: move to top/up, delete, transfer to
// the other box, move down/bottom. Bound to an ItemStorageBox (box) and its
// ItemStorageModel (model).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.ItemStorageBox
import App.ItemStorageModel

import "../../header"
import "../../general"
import "../../screens/home"

Rectangle {
  id: top
  property string title: ""
  property ItemStorageModel model: null
  property ItemStorageBox box: null

  // ---- Header bar: [check-all] Title (count/max), centered as one group ----
  Rectangle {
    id: bagHeader
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: 45

    color: brg.settings.accentColor
    Material.foreground: brg.settings.textColorLight
    Material.background: brg.settings.accentColor

    // Check-all parked at the left, nudged right so its icon centers over the
    // row checkboxes below (list inset 15 + a touch to clear the button's 6px
    // padding and match the Material checkbox indicator).
    IconButtonSquare {
      anchors.left: parent.left
      anchors.leftMargin: 24
      anchors.verticalCenter: parent.verticalCenter
      icon.source: "qrc:/assets/icons/fontawesome/check-double.svg"
      onClicked: model.checkedToggleAll()
    }

    // Title centered in the bar, with the live count just to its right.
    Text {
      id: titleText
      anchors.centerIn: parent
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      text: title
      font.pixelSize: 18
      color: brg.settings.textColorLight

      Text {
        anchors.left: parent.right
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        text: `(${box.itemsCount}/${box.itemsMax})`
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }
    }
  }

  ItemBoxView {
    id: bagView

    model: top.model
    box: top.box

    anchors.top: bagHeader.bottom
    anchors.bottom: bagFooter.top
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.leftMargin: 15
    anchors.rightMargin: 15
  }

  // ---- Footer bar: bulk actions, shown only while items are checked ----
  Rectangle {
    id: bagFooter
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    height: 45

    color: brg.settings.accentColor
    Material.foreground: brg.settings.textColorLight
    Material.background: brg.settings.accentColor

    RowLayout {
      anchors.centerIn: parent
      spacing: 15

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-double-up.svg"
        onClicked: model.checkedMoveToTop();
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-up.svg"
        onClicked: model.checkedMoveUp();
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
        onClicked: model.checkedDelete();
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/exchange-alt.svg"
        onClicked: model.checkedTransfer();
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-down.svg"
        onClicked: model.checkedMoveDown();
      }

      IconButtonSquare {
        visible: model.hasChecked
        icon.source: "qrc:/assets/icons/fontawesome/angle-double-down.svg"
        onClicked: model.checkedMoveToBottom();
      }
    }
  }
}
