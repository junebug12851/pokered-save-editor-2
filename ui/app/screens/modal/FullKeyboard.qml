import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/modal"
import "../../fragments/controls/name"

Page {
  id: top

  signal toggleExample();
  signal reUpdateExample();
  signal preClose();

  property string placeholder: "%%"
  property string str: ""
  property bool hasBox: false
  property bool is2Line: false
  property bool isPersonName: false
  property bool isPlayerName: false

  // Doesn't update for some reason on it's on. Used to, doesn't anymore, no
  // idea why.
  onStrChanged: {
    nameDisplay.str = str;
  }

  header: ToolBar {
    height: 75
    Material.background: Qt.lighter(brg.settings.accentColor, 1.50)

    Row {
      anchors.centerIn: parent
      spacing: 20

      Row {
        spacing: -16

        NameEdit {
          id: nameEdit
          text: top.str
          onTextChanged: {
            if(brg.fonts.countSizeOf(text) <= 10)
              top.str = text;
          }

          selectedColor: brg.settings.accentColor

          width: nameDisplay.chopLen * nameDisplay.sizeMult * 8

          disableAcceptBtn: true
          disableKeyboardBtn: true
          disableMenu: true

          topInset: 52
        }

        // Allows taking extra actions
        IconButtonSquare {
          id: menuBtn

          icon.width: 7
          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

          onClicked: menu.open();

          NameDisplayMenuNoTileset {
            id: menu

            isPersonName: top.isPersonName
            hasBox: top.hasBox

            onChangeStr: top.str = val;
            onToggleExample: top.toggleExample();
            onReUpdateExample: top.reUpdateExample();
          }
        }
      }

      Button {
        text: "Is Outdoor"
        opacity: (brg.settings.previewOutdoor) ? 1.00 : 0.50
        onClicked: brg.settings.previewOutdoor = !brg.settings.previewOutdoor;
        font.capitalization: Font.Capitalize
        font.pixelSize: 12
        flat: true
      }

      ComboBox {

        font.capitalization: Font.Capitalize
        font.pixelSize: 12
        flat: true
        width: font.pixelSize * 12

        model: [
          "Cavern",
          "Cemetery",
          "Club",
          "Dojo",
          "Facility",
          "Forest",
          "Forest Gate",
          "Gate",
          "Gym",
          "House",
          "Interior",
          "Lab",
          "Lobby",
          "Mansion",
          "Mart",
          "Museum",
          "Overworld",
          "Plateau",
          "Pokecenter",
          "Reds House 1",
          "Reds House 2",
          "Ship",
          "Ship Port",
          "Underground"
        ]

        onActivated: brg.settings.previewTileset = currentValue;
        Component.onCompleted: currentIndex = brg.settings.previewTilesetIndex;
      }
    }

    ModalClose {
      onClicked: {
        preClose();
        brg.router.closeScreen();
      }
      anchors.topMargin: 3
      anchors.rightMargin: 3
      icon.width: 28
      icon.height: 28
    }
  }

  footer: ToolBar {
    height: (top.hasBox) ? nameDisplay.height + 25 + (8 * 2) : 75
    Material.background: Qt.lighter(brg.settings.accentColor, 1.50)

    ColumnLayout {
      anchors.fill: parent
      spacing: -30

      NameDisplay {
        id: nameDisplay

        Layout.alignment: Qt.AlignHCenter
        placeholder: top.placeholder
        str: top.str
        hasBox: top.hasBox
        is2Line: top.is2Line
        isPersonName: top.isPersonName
        isPlayerName: top.isPlayerName

        enableFeedback: false
        disableEditor: true
        disableAutoPlaceholder: true
      }

      Text {
        Layout.alignment: Qt.AlignHCenter

        // Show this when the editor is visible and the name is an acceptable length
        visible: (nameDisplay.chopLen <= ((isPersonName) ? 7 : 10)) &&
                 (nameDisplay.regCount <= ((isPersonName) ? 7 : 10)) &&
                 (nameDisplay.expandedCount <= ((isPersonName) ? 7 : 10))

        font.pixelSize: 11
        color: "#424242"

        text: "Using " + nameDisplay.regCount + " out of " + nameDisplay.chopLen + " bytes."
      }

      // For person names only
      // Anyhting above 7 characters but below or equal to 10 can technically be
      // saved but your breaking the intended length
      Text {
        Layout.alignment: Qt.AlignHCenter

        // Show this when the editor is visible, it's a persons name, and we've
        // gone over 7 characters
        visible: nameDisplay.isPersonName &&
                 nameDisplay.expandedCount > 7 &&
                 nameDisplay.expandedCount <= 10

        font.pixelSize: 11
        color: "#BF360C"

        text: "Warning: This name is meant to be a max of 7 characters"
      }

      // Whenever the name unfolds much larger on the screen taking up significant
      // amount of tiles
      Text {
        Layout.alignment: Qt.AlignHCenter

        // Show this when the editor is visible, it's a persons name, and we've
        // gone over 7 characters
        visible: nameDisplay.expandedCount > 10

        font.pixelSize: 11
        color: "#BF360C"

        text: "Warning: This name expands out to be much longer on screen."
      }
    }
  }
}
