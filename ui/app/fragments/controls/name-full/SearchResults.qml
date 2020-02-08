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

  // There can only be one color and many fonts belong to more than one category
  function determineColor(ind) {
    let font = brg.fonts.fontAt(ind);

    if(font.normal)
      return brg.settings.fontColorNormal;
    else if(font.control)
      return brg.settings.fontColorControl;
    else if(font.picture)
      return brg.settings.fontColorPicture;
    else if(font.singleChar)
      return brg.settings.fontColorSingle;
    else if(font.variable)
      return brg.settings.fontColorVar;
    else if(font.multiChar)
      return brg.settings.fontColorMulti;

    // Error
    return "red";
  }

  delegate: Button {
    width: parent.width
    flat: true
    text: (brg.fonts.fontAt(fontInd).alias !== "")
          ? brg.fonts.fontAt(fontInd).alias
          : brg.fonts.fontAt(fontInd).name
    font.capitalization: Font.MixedCase

    Material.foreground: determineColor(fontInd)

    hoverEnabled: true
    onClicked: {
      var fontName = brg.fonts.fontAt(fontInd).name;
      var curStr = topz.str.toString();

      topz.str = curStr + fontName;
    }

    onHoveredChanged: {
      if(!hovered) {
        detailView.colorCodeEl.color = "transparent"
        detailView.titleEl.text = ""
        detailView.codeEl.text = ""
        detailView.descDividerEl.visible = false
        detailView.descEl.text = ""
        return;
      }

      detailView.colorCodeEl.color = determineColor(fontInd);
      detailView.titleEl.text = (brg.fonts.fontAt(fontInd).alias !== "")
          ? brg.fonts.fontAt(fontInd).alias
          : brg.fonts.fontAt(fontInd).name;
      detailView.codeEl.text = (brg.fonts.fontAt(fontInd).alias !== "")
          ? brg.fonts.fontAt(fontInd).name
          : "";
      detailView.descDividerEl.visible = brg.fonts.fontAt(fontInd).tip !== ""
      detailView.descEl.text = brg.fonts.fontAt(fontInd).tip;
    }
  }
}
