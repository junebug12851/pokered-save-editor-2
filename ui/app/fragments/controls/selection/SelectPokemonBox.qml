import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

ComboBox {
  id: control
  textRole: "boxName"
  valueRole: "boxValue"

  font.capitalization: Font.Capitalize
  font.pixelSize: 14
  flat: true

  width: font.pixelSize * 15

  delegate: ItemDelegate {
    width: control.width
    enabled: !boxDisabled

    contentItem: Text {
      text: boxName
      font: control.font
      color: (!boxDisabled)
             ? brg.settings.textColorDark
             : brg.settings.textColorMid
      verticalAlignment: Text.AlignVCenter
    }

    highlighted: control.highlightedIndex === index
  }

  popup: Popup {
    y: control.height - 1
    width: control.width
    implicitHeight: contentItem.implicitHeight
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
