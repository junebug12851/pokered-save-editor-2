// StructKey.qml -- a structural (non-tile) key on the keyboard deck.
//
// Shift / Ctrl / Alt / Backspace / Enter. Same silhouette and press feel as a KeyCap
// but neutral grey and carrying no game tile -- so the eye never confuses "a key that
// does something" with "a key that types something".
//
// A modifier key lights up in the accent colour while it's held OR latched, which is
// what makes the deck itself show you which page you're on.
import QtQuick

Item {
  id: top

  property string label: ""
  property real unit: 48      // the deck's key unit, in px
  property real units: 1      // width, in key units
  property bool active: false // held/latched modifier

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

    color: {
      if(top.active)
        return brg.settings.accentColor;
      if(mouse.pressed)
        return Qt.darker(brg.settings.dividerColor, 1.15);
      if(mouse.containsMouse)
        return Qt.lighter(brg.settings.dividerColor, 1.10);

      return Qt.lighter(brg.settings.dividerColor, 1.20);
    }

    border.width: 1
    border.color: top.active
                  ? Qt.darker(brg.settings.accentColor, 1.2)
                  : Qt.rgba(0, 0, 0, 0.12)

    Behavior on color { ColorAnimation { duration: 90 } }

    y: (mouse.pressed ? 3 : 2)
    Behavior on y { NumberAnimation { duration: 60 } }

    Text {
      anchors.centerIn: parent
      text: top.label
      font.pixelSize: Math.max(9, Math.round(top.unit * 0.2))
      font.bold: true
      color: top.active ? brg.settings.textColorLight : brg.settings.textColorDark
    }

    MouseArea {
      id: mouse
      anchors.fill: parent
      hoverEnabled: true
      cursorShape: Qt.PointingHandCursor
      onClicked: top.press();
    }

    SequentialAnimation {
      id: pressAnim
      NumberAnimation { target: face; property: "scale"; to: 0.94; duration: 55 }
      NumberAnimation { target: face; property: "scale"; to: 1.0;  duration: 85 }
    }
  }
}
