// SelectStatus.qml -- the status-condition picker combo (a borderless Select*
// combo).
//
// A flat, borderless ComboBox over brg.statusSelectModel (statusName/statusInd)
// with a hover underline and a height-capped, scrollable popup. Often placed on
// the accent header bar, so callers override hoverColor (see GlancePane). Shares
// the common Select* pattern (see SelectItem.qml).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ComboBox {
  id: control
  textRole: "statusName"
  valueRole: "statusInd"

  font.capitalization: Font.Capitalize
  font.pixelSize: 14
  flat: true
  model: brg.statusSelectModel

  width: font.pixelSize * 8

  // Hover underline; defaults blue-ish (accent). Header-bar instances override
  // hoverColor to a light color to match white-text header bars.
  property color hoverColor: brg.settings.accentColor
  // (SelectStatus lives on the accent header bar — GlancePane overrides hoverColor.)

  // Borderless at rest; a short underline on hover signals interactivity.
  background: Rectangle {
    color: "transparent"
    Rectangle {
      anchors.bottom: parent.bottom
      width: parent.width
      height: 2
      visible: control.hovered
      color: control.hoverColor
    }
  }

  delegate: ItemDelegate {
    width: control.width

    contentItem: Text {
      text: statusName
      font: control.font
      color: brg.settings.textColorDark
      verticalAlignment: Text.AlignVCenter
    }

    highlighted: control.highlightedIndex === index
  }

  popup: Popup {
    y: control.height - 1
    width: control.width
    // Cap the popup so long lists scroll instead of growing to full content
    // height (at full height there is nothing to flick and it clips off-screen).
    // +2 covers the 1px padding each side. Mirrors Qt's default ComboBox popup.
    height: Math.min(contentItem.implicitHeight + 2, 280)
    padding: 1

    contentItem: ListView {
      clip: true
      implicitHeight: contentHeight
      model: control.popup.visible ? control.delegateModel : null
      currentIndex: control.highlightedIndex

      ScrollBar.vertical: ScrollBar { }
    }
  }
}
