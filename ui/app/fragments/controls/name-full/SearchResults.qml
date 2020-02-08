import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

ListView {
  id: topz

  property string str: ""

  clip: true
  model: brg.fontSearchModel
  ScrollBar.vertical: ScrollBar {}

  delegate: Button {
    width: parent.width
    flat: true
    text: (brg.fonts.fontAt(fontInd).alias !== "")
          ? brg.fonts.fontAt(fontInd).alias
          : brg.fonts.fontAt(fontInd).name
    font.capitalization: Font.MixedCase

    hoverEnabled: true
    onClicked: {
      var fontName = brg.fonts.fontAt(fontInd).name;
      var curStr = topz.str.toString();

      topz.str = curStr + fontName;
    }
  }
}
