import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

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
