// NameFullHeader.qml -- the full keyboard's top bar.
//
// Just the name row now: a MODE indicator on the left, the NameFullEdit field centred,
// and a ModalClose that emits preClose() before closing (so the value commits exactly
// once). Carries the shared str/chopLen/sizeMult/isPersonName/hasBox state down.
//
// The "Simulated" tileset controls used to live up here, floating over a name field they
// have nothing to do with. They moved to SimulatedBar, which sits directly above the
// keyboard on the tile pages -- next to the only thing they affect, where they explain
// themselves. And the sentence that used to sit under the field ("Keyboard mode — type or
// click the keys...") is gone: it was long, it was ugly, and the mode icon says it
// instantly.
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
  // bars were eating half the screen: the keyboard is the point of this page, and it was
  // being squeezed into what was left.
  height: 74
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

  // ---- The name row, centred ----
  NameFullEdit {
    id: editor

    anchors.centerIn: parent
    // A touch of headroom: the row was sitting hard against the top of the bar.
    anchors.verticalCenterOffset: 2

    chopLen: top.chopLen
    sizeMult: top.sizeMult
    isPersonName: top.isPersonName
    hasBox: top.hasBox

    str: top.str
    onStrChanged: top.str = str;

    onEditStarted: top.editStarted();
    onEditEnded: top.editEnded();
  }

  // ---- The MODE indicator ----
  // Keyboard icon = the deck is live and owns your keystrokes. Pen = you're editing the
  // text directly and the keyboard is off.
  //
  // It is CENTRED in the gutter to the left of the name row -- both ways. Anchored to the
  // left edge it just hung there; centred in its own space it looks placed.
  Item {
    id: modeIcon

    width: 40
    height: 40

    x: Math.max(12, (editor.x - width) / 2)
    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: 2

    Rectangle {
      anchors.fill: parent
      radius: 6
      color: top.editMode
             ? Qt.lighter(brg.settings.accentColor, 1.75)
             : Qt.lighter(brg.settings.dividerColor, 1.28)
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
            ? qsTr("Edit mode — the keyboard is off. ✓ keeps your edit, ✗ discards it.")
            : qsTr("Keyboard mode — type or click the keys. The pen edits the text directly.")
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
