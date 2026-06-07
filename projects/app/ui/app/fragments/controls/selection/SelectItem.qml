// SelectItem.qml -- the item picker combo (one of the borderless Select* combos).
//
// A flat, borderless ComboBox over brg.itemSelectModel (itemSelectName/
// itemSelectInd) with a hover underline and a height-capped, scrollable popup.
// Entries with a negative index are section/disabled rows (greyed out). All the
// Select* combos in this folder share this exact pattern -- only the model, roles,
// and width differ.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ComboBox {
  id: control
  textRole: "itemSelectName"
  valueRole: "itemSelectInd"

  font.capitalization: Font.Capitalize
  font.pixelSize: 14
  flat: true
  model: brg.itemSelectModel

  width: font.pixelSize * 15

  // Hover underline; defaults blue-ish (accent). Header-bar instances override
  // hoverColor to a light color to match white-text header bars.
  property color hoverColor: brg.settings.accentColor

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
    enabled: itemSelectInd >= 0;

    contentItem: Text {
      text: itemSelectName
      font: control.font
      color: (itemSelectInd >= 0)
             ? brg.settings.textColorDark
             : brg.settings.textColorMid
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
