import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import app.itemstoragebox 1.0

import "../../general"
import "../../controls/selection"

ListView {
  id: itemBoxView
  property ItemStorageBox box: null

  clip: true
  ScrollBar.vertical: ScrollBar {}

  delegate: ColumnLayout {
    property bool isLast: index+1 < itemBoxView.count ? false : true

    width: parent.width

    Row {
      id: rowEntry
      Layout.alignment: Qt.AlignHCenter

      SelectItem {
        onActivated: itemId = currentValue;
        Component.onCompleted: {
          currentIndex = indexOfValue(itemId);
        }
      }

      DefTextEdit {
        id: countEdit

        topPadding: 13

        maximumLength: 2
        width: font.pixelSize * 2

        onTextChanged: {
          if(text === "")
            return;

          var dec = parseInt(text, 10);
          if(dec === NaN)
            return;

          if(dec < 0 || dec > 99)
            return;

          itemCount = dec;
        }

        Component.onCompleted: text = itemCount
      }

      IconButtonSquare {
        flat: true
        icon.color: "black"
        //Material.background: "red"
        icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
        onClicked: box.itemRemove(index);
      }
    }

    Button {
      Layout.alignment: Qt.AlignHCenter

      display: AbstractButton.IconOnly
      flat: true
      implicitWidth: rowEntry.implicitWidth

      visible: (isLast || box.itemsCount == 0) && (box.itemsCount < box.itemsMax)

      //Material.foreground: brg.settings.textColorLight
      Material.foreground: brg.settings.primaryColor

      icon.source: "qrc:/assets/icons/fontawesome/plus.svg"

      onClicked: box.itemNew()
    }

    Text {
      visible: isLast
      bottomPadding: 25
    }
  }
}
