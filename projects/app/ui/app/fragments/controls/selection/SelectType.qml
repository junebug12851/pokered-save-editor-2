import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ComboBox {
  property bool type2: false

  id: control
  textRole: "typeName"
  valueRole: "typeInd"

  model: brg.typesModel

  font.capitalization: Font.Capitalize
  font.pixelSize: 14
  flat: true

  width: font.pixelSize * 8

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
    enabled: typeInd < 0xFF || type2;

    contentItem: Text {
      text: typeName
      font: control.font
      color: (typeInd < 0xFF || type2)
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
