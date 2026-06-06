import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.ItemStorageBox

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
    id: delegate
    property bool isLast: index+1 < itemBoxView.count ? false : true

    width: parent ? parent.width : 0

    Row {
      id: rowEntry
      Layout.alignment: Qt.AlignHCenter
      visible: !itemIsPlaceholder

      CheckBox {
        id: selectBox

        Component.onCompleted: (itemChecked !== undefined)
                               ? checked = itemChecked
                               : checked = false;

        onCheckedChanged: itemChecked = checked;
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

        // Center the count text vertically (the old fixed top-padding nudge no
        // longer lines up with Qt 6 field heights). Items in a Row top-align, and
        // this field's height matches the SelectItem dropdown, so centering the
        // text lines it up with the dropdown's text.
        verticalAlignment: TextInput.AlignVCenter

        maximumLength: 2
        width: 2 * font.pixelSize + leftPadding + rightPadding

        onTextChanged: {
          if(text === "")
            return;

          var dec = parseInt(text, 10);
          if(isNaN(dec))
            return;

          if(dec < 0 || dec > 99)
            return;

          itemCount = dec;
        }

        Component.onCompleted: (itemCount !== undefined)
                               ? text = itemCount
                               : text = "";
      }
    }

    Button {
      Layout.alignment: Qt.AlignHCenter

      display: AbstractButton.IconOnly
      flat: true
      implicitWidth: rowEntry.implicitWidth

      visible: itemIsPlaceholder && (box.itemsCount < box.itemsMax)

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
