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
  height: 80
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

  // The "Simulated" group's pill toggle. Rounded, properly PADDED (it read as a bare
  // word crammed in a box before -- 5px of side padding and none to speak of on top),
  // and it fills with the accent when on.
  component FlatToggle: Button {
    id: ftb
    property bool active: false

    flat: true
    font.capitalization: Font.Capitalize
    font.pixelSize: 11
    Material.elevation: 0

    topPadding: 6
    bottomPadding: 6
    leftPadding: 12
    rightPadding: 12

    background: Rectangle {
      radius: 4
      border.width: 1
      border.color: ftb.active
                    ? brg.settings.accentColor
                    : brg.settings.dividerColor
      color: ftb.active
             ? brg.settings.accentColor
             : (ftb.hovered ? Qt.lighter(brg.settings.dividerColor, 1.18) : "#ffffff")
    }

    contentItem: Text {
      text: ftb.text
      font: ftb.font
      color: ftb.active ? brg.settings.textColorLight : brg.settings.textColorDark
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
  }

  // ---- The MODE indicator, top-left ----
  // Replaces the sentence that used to sit under the field ("Keyboard mode — type or
  // click the keys..."): it was long, it was ugly, and it said in twenty words what an
  // icon says instantly. Keyboard icon = the deck is live. Pen = you're editing the text
  // directly and the keyboard is off.
  Item {
    id: modeIcon

    anchors.left: parent.left
    anchors.leftMargin: 14
    anchors.verticalCenter: parent.verticalCenter
    width: 40
    height: 40

    Rectangle {
      anchors.fill: parent
      radius: 6
      color: top.editMode
             ? Qt.lighter(brg.settings.accentColor, 1.75)
             : Qt.lighter(brg.settings.dividerColor, 1.25)
      border.width: 1
      border.color: top.editMode
                    ? brg.settings.accentColor
                    : brg.settings.dividerColor

      Behavior on color { ColorAnimation { duration: 140 } }
    }

    Image {
      anchors.centerIn: parent
      source: top.editMode
              ? "qrc:/assets/icons/fontawesome/pen.svg"
              : "qrc:/assets/icons/fontawesome/keyboard.svg"
      sourceSize.width: top.editMode ? 17 : 22
      sourceSize.height: top.editMode ? 17 : 22
      opacity: 0.85
    }

    HoverHandler { id: modeHover }

    MainToolTip {
      followGlobalSetting: false
      visible: modeHover.hovered
      text: top.editMode
            ? qsTr("Edit mode — the keyboard is off. ✓ applies your edit, ✗ discards it.")
            : qsTr("Keyboard mode — type or click the keys. The pen edits the text directly.")
    }
  }

  ColumnLayout {
    anchors.centerIn: parent
    spacing: 6

    // ---- "Simulated" control group ----
    // The label + the two controls now live in ONE tray, so they read as a single group
    // that belongs together rather than three things loose at the top of the bar. (They
    // were: a bold word, a toggle with no padding, and a bare combo, all floating.)
    Rectangle {
      Layout.alignment: Qt.AlignHCenter
      implicitWidth: simRow.implicitWidth + 22
      implicitHeight: simRow.implicitHeight + 10

      radius: 6
      color: Qt.lighter(brg.settings.dividerColor, 1.34)
      border.width: 1
      border.color: brg.settings.dividerColor

      RowLayout {
        id: simRow
        anchors.centerIn: parent
        spacing: 9

        // Plain caption — NOT a button.
        Label {
          text: qsTr("Simulated")
          font.bold: true
          font.pixelSize: 11
          color: brg.settings.textColorMid
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

