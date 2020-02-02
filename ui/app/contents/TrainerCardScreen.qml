import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

Rectangle {

  // image://tileset/<tileset>/<type>/<font>/<frame>/<tile>/<width>/<height>
  //    image://font/<tileset>/<type>/<frame>/<width>/<height>/<box>/<2-lines>/<max>/<bgColor>/<fgColor>/<placeholder>/<str>

  property int curFrame: 0
  property int tick: 1000 / 3

  property int sizeMult: 4

  color: "blue"

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

//  Image {
//    source: "image://tileset/overworld/outdoor/font/" + curFrame + "/whole/" + width + "/" + height
//    width: 500
//    height: 500
//  }

  Image {
    source: "image://font/overworld/outdoor/0/" + width + "/" + height + "/box/1-line/7/transparent/none/You are <line> _str_/June"
    width: (8 * 20) * sizeMult
    height: (8 * 6) * sizeMult
    cache: false
  }
}
