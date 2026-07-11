// NameFullHeader.qml -- the full keyboard's top toolbar.
//
// Hosts the "Simulated" controls (Outdoor toggle + NameFullTileset combo -- they
// pick which tileset the keys' animated glyphs are rendered from), the NameFullEdit
// name field, and a ModalClose that emits preClose() before closing (so the value
// commits exactly once). Carries the shared str/chopLen/sizeMult/isPersonName/hasBox
// state down to the editor.
//
// The old Grid/Tileset toggle is GONE with the raw tilemap view it drove -- the deck
// shows every tile now, so there is nothing left for a second view to add.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../name"
import "../../general"
import "../../modal"

ToolBar {
  id: top

  signal preClose();

  property string str: ""
  property int chopLen: 0
  property int sizeMult: 0
  property bool isPersonName: false
  property bool hasBox: false

  // Which of the screen's two modes we're in. Owned by the editor row (it holds the
  // pen / check / cross), surfaced here so FullKeyboard can fade + disable the deck.
  readonly property bool editMode: editor.editMode

  signal editStarted()
  signal editEnded()

  // Shake the name field: the deck calls this when a key won't fit.
  function reject() {
    editor.reject();
  }

  // Slim, and a clean surface -- NOT the old washed-out `lighter(accent, 1.5)` blue
  // stripe. That colour was ugly on its own and, at 132px plus a 119px footer, the two
  // bars were eating half the screen: the keyboard is the point of this page, and it
  // was being squeezed into what was left. The bar is now a light surface with a hairline
  // divider, and it earns its height back for the deck.
  height: 88
  Material.background: brg.settings.textColorLight

  // The hairline that separates the bar from the body (replacing the colour block).
  Rectangle {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    height: 1
    color: brg.settings.dividerColor
  }

  onStrChanged: editor.str = top.str

  // Flat, square toggle button: no Material elevation/shadow, no rounded
  // corners. Filled with the accent when `active`, outlined otherwise.
  component FlatToggle: Button {
    id: ftb
    property bool active: false

    flat: true
    font.capitalization: Font.Capitalize
    font.pixelSize: 11
    Material.elevation: 0

    topPadding: 4
    bottomPadding: 4
    leftPadding: 6
    rightPadding: 6

    background: Rectangle {
      radius: 0
      border.width: 1
      border.color: brg.settings.accentColor
      color: ftb.active
             ? brg.settings.accentColor
             : (ftb.hovered ? Qt.lighter(brg.settings.accentColor, 1.65) : "transparent")
    }

    contentItem: Text {
      text: ftb.text
      font: ftb.font
      color: ftb.active ? brg.settings.textColorLight : brg.settings.textColorDark
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
  }

  ColumnLayout {
    anchors.centerIn: parent
    spacing: 3

    // ---- "Simulated" control group: a plain text label, then two toggle
    //      buttons and the tileset combo. ----
    RowLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 3

      // Plain caption — NOT a button.
      Label {
        text: qsTr("Simulated")
        font.bold: true
        font.pixelSize: 11
        color: brg.settings.textColorDark
        Layout.alignment: Qt.AlignVCenter
      }

      // Bold vertical pipe between the label and the controls (Twilight likes it).
      ToolSeparator {
        Layout.fillHeight: false
        Layout.preferredHeight: 18
        Layout.alignment: Qt.AlignVCenter
      }

      FlatToggle {
        text: qsTr("Outdoor")
        active: brg.settings.previewOutdoor
        onClicked: brg.settings.previewOutdoor = !brg.settings.previewOutdoor;

        MainToolTip { text: "Render tiles as they'd look outdoors vs. indoors." }
      }

      NameFullTileset {
        Layout.alignment: Qt.AlignVCenter
        // In a RowLayout the combo's internal `width` is ignored, so without a
        // preferred width it collapses to its indicator and the tileset name
        // doesn't show. Keep it just wide enough for the names.
        Layout.preferredWidth: 132
      }
    }

    // ---- Name input: wide, centered, with its mode buttons beside it ----
    NameFullEdit {
      id: editor
      Layout.alignment: Qt.AlignHCenter

      chopLen: top.chopLen
      sizeMult: top.sizeMult
      isPersonName: top.isPersonName
      hasBox: top.hasBox

      str: top.str
      onStrChanged: top.str = str;

      onEditStarted: top.editStarted();
      onEditEnded: top.editEnded();
    }

    // ---- Which mode you're in, said out loud ----
    // The keyboard fading out is the loud signal; this is the one that names it, so
    // nobody has to infer the rule from the animation.
    Label {
      Layout.alignment: Qt.AlignHCenter

      text: top.editMode
            ? qsTr("Edit mode — keyboard off. ✓ applies, ✗ discards.")
            : qsTr("Keyboard mode — type or click the keys. ✎ edits the text directly.")

      font.pixelSize: 10
      color: brg.settings.textColorDark
      opacity: 0.75
    }
  }

  ModalClose {
    onClicked: {
      top.preClose();
      brg.router.closeScreen();
    }
    anchors.topMargin: 3
    anchors.rightMargin: 3
    icon.width: 28
    icon.height: 28
  }
}

