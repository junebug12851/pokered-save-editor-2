import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.ItemStorageBox 1.0

import "../../general"
import "../../controls/selection"

ListView {
  id: itemBoxView
  property ItemStorageBox box: null

  clip: true
  ScrollBar.vertical: ScrollBar {}

  // Hack because insert doesn't work as exoected
  function hack_newAndRePositionViewEnd() {
    box.itemNew();
    positionViewAtEnd();
  }

  delegate: ColumnLayout {
    property bool isLast: index+1 < itemBoxView.count ? false : true

    width: parent.width

    Row {
      id: rowEntry
      Layout.alignment: Qt.AlignHCenter
      visible: (!isLast && box.itemsCount > 0)

      IconButtonSquare {
        visible: !box.isBag
        //enabled: !box.relocateFull // Works only sometimes for some reason
        icon.source: "qrc:/assets/icons/fontawesome/angle-left.svg"
        onClicked: box.relocateOne(index)
      }

      SelectItem {
        onActivated: itemId = currentValue;
        Component.onCompleted: {

          // WAAAAYYYY FASTER
          // I have no idea why the native function to do this is a million times
          // slower. It's a billion times faster to implement my own in C++
          // and I'm not even dealing with that many rows.
          currentIndex = brg.itemSelectModel.itemToListIndex(itemId);

          // EXTREMELY SLOW
          //currentIndex = indexOfValue(itemId);
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

        Component.onCompleted: (itemCount !== undefined)
                               ? text = itemCount
                               : text = "";
      }

      IconButtonSquare {
        icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
        onClicked: box.itemRemove(index);
      }

      IconButtonSquare {
        visible: box.isBag
        //enabled: !box.relocateFull // Works only sometimes for some reason
        icon.source: "qrc:/assets/icons/fontawesome/angle-right.svg"
        onClicked: box.relocateOne(index)
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

      onClicked: {
        // A hack because insert doesn't work as expected and I have no idea why
        // after a lot of research
        //itemBoxView.hack_rePositionViewEnd();
        hack_newAndRePositionViewEnd();
      }
    }

    Text {
      visible: isLast
      bottomPadding: 25
    }
  }
}
