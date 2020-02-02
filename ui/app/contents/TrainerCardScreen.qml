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
  property int sizeMult: 2 //4

  property string tileset: "overworld"
  property string type: "outdoor"
  property string placeholder: "%%"
  property string str: "<pic03>12345678<pic14>"
  property string hasBox: "box"
  property string is2Line: "2-lines"
  property int maxLen: 255
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

  Image {
    id: wholeTileset
    source: "image://tileset/" +
            tileset + "/" +
            type +
            "/font/" +
            curFrame +
            "/whole/" +
            width + "/" +
            height

    width: (8 * 16) * sizeMult
    height: (8 * 16) * sizeMult
  }

  Image {
    anchors.left: wholeTileset.left
    anchors.top: wholeTileset.bottom
    anchors.topMargin: 10

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
            util.encodeBeforeUrl(placeholder) +"/" +
            util.encodeBeforeUrl(str)

    width: (8 * 20) * sizeMult
    height: (8 * 6) * sizeMult
    cache: false
  }
}
