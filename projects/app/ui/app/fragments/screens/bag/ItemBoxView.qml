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

  // Per-control height knobs (Qt 6 Material controls each have a different
  // implicit height; pin them so the checkbox / combo / count field share one
  // consistent, vcentered row — see notes/reference/ui-patterns.md).
  property int rowH: 38     // the row's height (the combo sets the tallest)
  property int comboH: 38   // SelectItem dropdown
  property int textH: 30    // count text box (a touch shorter)

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

    RowLayout {
      id: rowEntry
      Layout.alignment: Qt.AlignHCenter
      spacing: 8
      visible: !itemIsPlaceholder

      CheckBox {
        id: selectBox

        // VCenter + minimumHeight:0 so the Material checkbox's ~40px touch
        // target doesn't floor the row or ride high (ui-patterns.md →
        // "Material controls fight small heights").
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredHeight: itemBoxView.rowH
        Layout.minimumHeight: 0

        Component.onCompleted: (itemChecked !== undefined)
                               ? checked = itemChecked
                               : checked = false;

        onCheckedChanged: itemChecked = checked;
      }

      SelectItem {
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredHeight: itemBoxView.comboH
        Layout.preferredWidth: font.pixelSize * 15

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

        // VCenter text + pin the field height so it lines up with the combo.
        // DefTextEdit already centers its text at any height (topPadding:0).
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredHeight: itemBoxView.textH
        Layout.preferredWidth: 2 * font.pixelSize + leftPadding + rightPadding
        verticalAlignment: TextInput.AlignVCenter

        maximumLength: 2

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
