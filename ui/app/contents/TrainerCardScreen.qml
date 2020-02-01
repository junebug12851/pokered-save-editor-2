import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

Rectangle {

  // image://tileset/<tileset>/<type>/<font>/<frame>/<tile>/<width>/<height>

  property int curFrame: 0
  property int tick: 1000 / 3

  Timer {
    interval: tick;
    running: true;
    repeat: true
    onTriggered: {
      if((curFrame + 1) >= 8)
        curFrame = 0;
      else
        curFrame++;
    }
  }

  Image {
    source: "image://tileset/overworld/outdoor/nofont/" + curFrame + "/whole/" + (width*2) + "/" + (height*2)
    width: 500
    height: 500
    antialiasing: false
    layer.enabled: false
  }
}
