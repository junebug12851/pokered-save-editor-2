import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

Rectangle {

  // image://tileset/<tileset>/<type>/<font>/<frame>/<tile>/<width>/<height>

  Image {
    anchors.fill: parent

    source: "image://tileset/house/outdoor/font/4/whole/" + width + "/" + height
    width: 400
    height: 400
  }
}
