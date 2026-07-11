// StructKey.qml -- a structural (non-tile) key on the keyboard deck.
//
// Caps / Shift / Ctrl / Alt / Backspace / Enter. Same silhouette and press feel as a
// KeyCap, but DARK against the chassis and carrying no game tile -- so the eye never
// confuses "a key that does something" with "a key that types something". (When these
// were the same light grey as the tile caps, the deck was one undifferentiated field of
// keys.)
//
// A modifier lights up bright while it's held OR latched, which is what makes the deck
// itself show you which page you're on.
import QtQuick

Item {
  id: top

  property string label: ""
  property real unit: 48      // the deck's key unit, in px
  property real units: 1      // width, in key units
  property bool active: false // held/latched modifier

  // A DEAD key: one of the real keyboard's keys that this deck doesn't use (Tab, the
  // bracket/punctuation keys, Win/Menu). It is drawn and does nothing.
  //
  // They're here purely so the deck reads as a KEYBOARD. Without them the thing was a
  // floating block of 36 caps: correct, roomier, and somehow worse to look at -- the
  // silhouette everyone recognises comes from the ragged edges (Tab, \, ;, ', /) as
  // much as from the letters. They're deliberately inert and muted: they take no
  // clicks, no hover, and no cursor, so they can't be mistaken for keys that type.
  property bool dead: false

  signal fired()

  /// Just the press animation -- for when the PHYSICAL key was pressed and the deck
  /// has already done the work itself.
  function flash() {
    pressAnim.restart();
  }

  function press() {
    top.flash();
    top.fired();
  }

  implicitWidth: unit * units
  implicitHeight: unit
  width: implicitWidth
  height: implicitHeight

  Rectangle {
    id: face
    anchors.fill: parent
    anchors.margins: 2
    radius: 6

    // Dark caps on the dark chassis; a held/latched modifier goes BRIGHT so the deck
    // announces its own state. A dead key sinks back into the body.
    color: {
      if(top.dead)
        return Qt.darker(brg.settings.accentColor, 1.42);
      if(top.active)
        return Qt.lighter(brg.settings.accentColor, 1.35);
      if(mouse.pressed)
        return Qt.darker(brg.settings.accentColor, 1.05);
      if(mouse.containsMouse)
        return Qt.darker(brg.settings.accentColor, 1.02);

      return Qt.darker(brg.settings.accentColor, 1.22);
    }

    border.width: 1
    border.color: top.active
                  ? Qt.lighter(brg.settings.accentColor, 1.6)
                  : Qt.darker(brg.settings.accentColor, 1.7)

    Behavior on color { ColorAnimation { duration: 90 } }

    y: (mouse.pressed ? 3 : 1)
    Behavior on y { NumberAnimation { duration: 60 } }

    // Shadow on the chassis, matching the tile caps.
    Rectangle {
      z: -1
      anchors.fill: parent
      anchors.topMargin: 2
      radius: parent.radius
      color: Qt.rgba(0, 0, 0, 0.22)
      visible: !mouse.pressed
    }

    Text {
      anchors.centerIn: parent
      text: top.label
      font.pixelSize: Math.max(8, Math.round(top.unit * 0.2))
      font.bold: !top.dead
      color: top.active
             ? Qt.darker(brg.settings.accentColor, 2.0)
             : brg.settings.textColorLight
      opacity: top.dead ? 0.30 : 1.0
    }

    MouseArea {
      id: mouse
      anchors.fill: parent
      enabled: !top.dead
      hoverEnabled: !top.dead
      cursorShape: top.dead ? Qt.ArrowCursor : Qt.PointingHandCursor
      onClicked: top.press();
    }

    SequentialAnimation {
      id: pressAnim
      NumberAnimation { target: face; property: "scale"; to: 0.94; duration: 55 }
      NumberAnimation { target: face; property: "scale"; to: 1.0;  duration: 85 }
    }
  }
}
