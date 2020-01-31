import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

Rectangle {

  //property FontSearch fs: new FontSearch

  color: Style.primaryColorLight
  anchors.fill: parent

  Text {
    id: something
    //text: fontSearch.
  }

  Component.onCompleted: {
    //fontSearch.andNormal()
    fontSearch.notMultiChar().andNormal()
    fontSearch.notShorthand()

    for(var i = 0; i < fontSearch.fontCount(); i++) {
      something.text += fontSearch.fontStrAt(i) + ", "
    }
  }
}
