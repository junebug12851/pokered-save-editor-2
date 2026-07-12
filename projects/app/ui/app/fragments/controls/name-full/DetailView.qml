// DetailView.qml -- the right-hand detail pane of the full keyboard.
//
// Fed by whichever key the mouse is over (KeyboardDeck's `detail` signal). Shows a
// render of the tile, its name, its raw code, its category, and its description.
//
// When nothing is hovered it does NOT go blank -- a pane that empties itself every time
// the mouse moves reads as broken. It falls back to a short "what am I looking at" note.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
  id: top

  // The key map from brg.keyboard.keyData(), or null when nothing is hovered.
  property var info: null

  readonly property bool has: info && info.empty === false

  clip: true

  function catColor(c) {
    switch(c) {
      case 1: return brg.settings.fontColorNormal;
      case 2: return brg.settings.fontColorSingle;
      case 3: return brg.settings.fontColorMulti;
      case 4: return brg.settings.fontColorVar;
      case 5: return brg.settings.fontColorPicture;
      case 6: return brg.settings.fontColorControl;
    }

    return brg.settings.dividerColor;
  }

  function catName(c) {
    switch(c) {
      case 1: return qsTr("Normal");
      case 2: return qsTr("Single-Char");
      case 3: return qsTr("Multi-Char");
      case 4: return qsTr("Variable");
      case 5: return qsTr("Picture");
      case 6: return qsTr("Control");
    }

    return "";
  }

  // The split divider: this pane is the other half of the screen, and the line says so.
  Rectangle {
    id: divider
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: 1
    color: brg.settings.dividerColor
  }

  ColumnLayout {
    anchors.left: divider.right
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.margins: 14
    spacing: 7

    // ---- The tile ----
    Rectangle {
      id: previewBox

      Layout.fillWidth: true
      Layout.preferredHeight: 64
      radius: 8

      color: top.has
             ? Qt.tint("#ffffff", Qt.rgba(top.catColor(top.info.category).r,
                                          top.catColor(top.info.category).g,
                                          top.catColor(top.info.category).b, 0.10))
             : Qt.rgba(0, 0, 0, 0.03)

      border.width: 1
      border.color: top.has
                    ? Qt.rgba(top.catColor(top.info.category).r,
                              top.catColor(top.info.category).g,
                              top.catColor(top.info.category).b, 0.4)
                    : Qt.rgba(0, 0, 0, 0.06)

      // A control code has no glyph to show at all -- so say what it does instead of
      // rendering a lie.
      TilePreview {
        id: preview
        anchors.centerIn: parent

        visible: top.has && top.info.render !== 3
        tileName: (top.has && top.info.render !== 3) ? top.info.code : ""

        // ONE BYTE CAN PRINT FIFTEEN CHARACTERS (<user> -> "Enemy <SPECIES>"). At a fixed
        // scale that render is wider than the whole pane and gets sliced off, so the
        // scale is fitted to the expansion: big for a single glyph, small for a sentence.
        sizeMult: {
          var w = previewBox.width - 16;
          var perTile = w / Math.max(1, preview.chop * 8);
          return Math.max(1, Math.min(4, perTile));
        }
      }

      Text {
        anchors.centerIn: parent
        visible: top.has && top.info.render === 3
        text: qsTr("text-engine code\n(prints nothing)")
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 11
        font.italic: true
        color: brg.settings.textColorMid
      }

      Text {
        anchors.centerIn: parent
        visible: !top.has
        text: qsTr("Hover a key")
        font.pixelSize: 12
        font.italic: true
        color: brg.settings.textColorMid
      }
    }

    // ---- Name ----
    Text {
      Layout.fillWidth: true
      visible: top.has
      text: top.has ? top.info.title : ""
      font.pixelSize: 16
      font.bold: true
      color: brg.settings.textColorDark
      wrapMode: Text.WordWrap
      maximumLineCount: 2
      elide: Text.ElideRight
    }

    // ---- Category + raw code ----
    RowLayout {
      Layout.fillWidth: true
      visible: top.has
      spacing: 6

      Rectangle {
        Layout.preferredWidth: catText.implicitWidth + 14
        Layout.preferredHeight: 20
        radius: 10
        color: top.has ? top.catColor(top.info.category) : "transparent"

        Text {
          id: catText
          anchors.centerIn: parent
          text: top.has ? top.catName(top.info.category) : ""
          font.pixelSize: 10
          font.bold: true
          color: brg.settings.textColorLight
        }
      }

      Text {
        Layout.fillWidth: true
        // Only worth showing when it differs from the name -- "a" is its own code.
        text: (top.has && top.info.code !== top.info.title) ? top.info.code : ""
        font.pixelSize: 12
        font.italic: true
        color: brg.settings.textColorMid
        textFormat: Text.PlainText
        elide: Text.ElideRight
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: 1
      visible: top.has && top.info.tip !== ""
      color: brg.settings.dividerColor
    }

    // ---- Description ----
    // SCROLLS. Some of these descriptions are long, and the pane is ~175px tall in the
    // app's default window: eliding them just hid the half that explained what the code
    // actually does to your save.
    Flickable {
      Layout.fillWidth: true
      Layout.fillHeight: true
      visible: top.has && top.info.tip !== ""

      clip: true
      contentWidth: width
      contentHeight: tipText.implicitHeight
      boundsBehavior: Flickable.StopAtBounds
      ScrollBar.vertical: ScrollBar { width: 6 }

      Text {
        id: tipText
        width: parent.width - 8
        text: top.has ? top.info.tip : ""
        font.pixelSize: 11
        color: brg.settings.textColorDark
        wrapMode: Text.WordWrap
        lineHeight: 1.2
      }
    }

    // ---- The idle note ----
    // ONE block, and factually right. "Each key holds one game character" was simply
    // wrong: a key holds one BYTE, and that byte might print one character, or several,
    // or a name pulled out of memory, or nothing at all while it does something to the
    // text engine.
    Text {
      Layout.fillWidth: true
      Layout.fillHeight: true
      visible: !top.has
      text: qsTr("Every key is one byte. That byte may print one character, several, a " +
                 "whole name — or nothing at all, and change the text instead.")
      font.pixelSize: 11
      color: brg.settings.textColorMid
      wrapMode: Text.WordWrap
      lineHeight: 1.3
      elide: Text.ElideRight
      verticalAlignment: Text.AlignTop
    }
  }
}
