import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import app.itemstoragebox 1.0

import "../../controls/selection"

ListView {
  property ItemStorageBox box: null

  clip: true
  ScrollBar.vertical: ScrollBar {}

  delegate: Rectangle {
    width: parent.width
    height: 50

    Row {
      anchors.fill: parent

      SelectItem {
        onActivated: itemId = currentValue;
        Component.onCompleted: {
          currentIndex = indexOfValue(itemId);
        }
      }

      TextEdit {
        width: 50
        onTextChanged: itemCount = parseInt(text, 10)
        Component.onCompleted: text = itemCount.toString()
      }

      Text {
        text: (itemWorthEach > 0)
              ? "$" + itemWorthAll + " (ea: $" + itemWorthEach + " )"
              : ""
      }

      Button {
        text: "Delete"
        onClicked: box.itemRemove(index);
      }
    }
  }
}
