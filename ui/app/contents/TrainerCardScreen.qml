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

  property string tileset: "overworld"
  property string type: "outside"
  property string placeholder: "%%"
  property string str: "12345678"
  property string hasBox: "no-box"
  property string is2Line: "1-line"
  property int maxLen: 7
  property string bgColor: "transparent"
  property string fgColor: "none"

  color: Style.primaryColorLight

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
    source: "image://font/" +
            tileset + "/" +
            type + "/" +
            curFrame + "/" +
            width + "/" +
            height + "/" +
            hasBox + "/" +
            is2Line + "/" +
            maxLen + "/" +
            bgColor + "/" +
            fgColor + "/" +
            placeholder +"/" +
            str

    width: (8 * 20) * sizeMult
    height: (8 * 6) * sizeMult
    cache: false
  }
}
