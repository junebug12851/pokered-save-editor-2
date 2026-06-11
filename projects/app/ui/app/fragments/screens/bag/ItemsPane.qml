// ItemsPane.qml -- one titled item box (used twice on the Bag screen).
//
// Wraps an ItemBoxView below a colored header bar (check-all toggle + title +
// live count/max). The old footer of bulk-action buttons (move to top/up/down/
// bottom, transfer, delete) was removed once drag & drop landed -- reordering and
// cross-pane moves are now direct drag gestures (ItemBoxView), and delete moved
// to a per-row hover/checked chip on the row. Bound to an ItemStorageBox (box)
// and its ItemStorageModel (model). See notes/reference/ui-patterns.md.
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
    // row checkboxes below. The rows now lead with a 24px grip handle (+8 spacing)
    // before the checkbox, so this clears that column too (was 24 before the grip
    // was added). Tuning knob -- adjust if the grip width/spacing changes.
    IconButtonSquare {
      anchors.left: parent.left
      anchors.leftMargin: 56
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
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.leftMargin: 15
    anchors.rightMargin: 15
  }
}
