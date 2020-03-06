import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

ComboBox {
  id: control
  textRole: "speciesName"
  valueRole: "speciesInd"

  font.capitalization: Font.Capitalize
  font.pixelSize: 14
  flat: true
  model: brg.speciesSelectModel

  width: font.pixelSize * 11

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
