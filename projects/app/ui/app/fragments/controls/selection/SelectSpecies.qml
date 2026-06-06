import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ComboBox {
  id: control
  textRole: "speciesName"
  valueRole: "speciesInd"

  font.capitalization: Font.Capitalize
  font.pixelSize: 14
  flat: true
  model: brg.speciesSelectModel

  width: font.pixelSize * 11

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
    enabled: speciesInd >= 0;

    contentItem: Text {
      text: speciesName
      font: control.font
      color: (speciesInd >= 0)
             ? brg.settings.textColorDark
             : brg.settings.textColorMid
      verticalAlignment: Text.AlignVCenter
    }

    highlighted: control.highlightedIndex === index
  }

  popup: Popup {
    y: control.height - 1
    width: control.width
    // Cap the popup so long lists (151 species, etc.) scroll instead of growing
    // to full content height — at full height the ListView == its content, so
    // there is nothing to flick and it just clips at the screen edge. +2 covers
    // the 1px padding on each side. Mirrors Qt's own default ComboBox popup.
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
