// ColorLegend.qml -- the keyboard's colour reference rail.
//
// Replaces the old category FILTER list. The deck already shows every tile, so there
// is nothing to filter -- what a user needs is to know what the key colours MEAN.
// So this is a legend, not a control: a swatch, the category's name, and an
// info dot that explains it on hover. Nothing to click, nothing to get wrong.
//
// The order is the category order (Normal -> Single -> Multi -> Variable -> Picture
// -> Control), which is also the order the pages run in -- and roughly the order from
// "always safe in a name" to "will glitch your game".
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"

Item {
  id: top

  implicitWidth: 118

  Column {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.topMargin: 4
    spacing: 0

    Text {
      text: qsTr("Key colours")
      font.pixelSize: 11
      font.bold: true
      color: brg.settings.textColorDark
      bottomPadding: 4
    }

    Repeater {
      model: [
        {
          name: qsTr("Normal"),
          color: brg.settings.fontColorNormal,
          help: "Characters you're allowed to pick in-game. These are guaranteed " +
                "to always look the same throughout gameplay."
        },
        {
          name: qsTr("Single-Char"),
          color: brg.settings.fontColorSingle,
          help: "Characters that take up 1 space on screen and in the save data."
        },
        {
          name: qsTr("Multi-Char"),
          color: brg.settings.fontColorMulti,
          help: "Characters that take up more than 1 space on screen yet only " +
                "take up 1 byte in the save data."
        },
        {
          name: qsTr("Variable"),
          color: brg.settings.fontColorVar,
          help: "Insert an existing name or other text from elsewhere in memory."
        },
        {
          name: qsTr("Picture"),
          color: brg.settings.fontColorPicture,
          help: "Tiles you can insert into the game; the majority of them will " +
                "change throughout gameplay. We approximate them here."
        },
        {
          name: qsTr("Control"),
          color: brg.settings.fontColorControl,
          help: "Special codes that control the text engine. Using these in a " +
                "name will cause glitching and trash on the screen."
        }
      ]

      Item {
        id: legendRow

        required property var modelData

        width: parent.width
        height: 25

        Rectangle {
          id: swatch
          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
          width: 14
          height: 14
          radius: 3

          // The same treatment the keys get -- a hairline border over a faint wash --
          // so the swatch reads as "that key", not as a different colour entirely.
          color: Qt.tint("#ffffff", Qt.rgba(legendRow.modelData.color.r,
                                            legendRow.modelData.color.g,
                                            legendRow.modelData.color.b, 0.30))
          border.width: 1
          border.color: legendRow.modelData.color
        }

        Text {
          anchors.left: swatch.right
          anchors.leftMargin: 7
          anchors.right: dot.left
          anchors.rightMargin: 4
          anchors.verticalCenter: parent.verticalCenter
          text: legendRow.modelData.name
          font.pixelSize: 11
          color: brg.settings.textColorDark
          elide: Text.ElideRight
        }

        // The info dot: explains its category ONLY while the dot itself is hovered,
        // so a mouse crossing the rail doesn't spray tooltips.
        Label {
          id: dot
          anchors.right: parent.right
          anchors.rightMargin: 4
          anchors.verticalCenter: parent.verticalCenter

          text: "ⓘ"
          font.pixelSize: 15
          color: brg.settings.textColorDark
          opacity: hh.hovered ? 1.0 : 0.5

          HoverHandler { id: hh }

          MainToolTip {
            followGlobalSetting: false
            visible: hh.hovered
            text: legendRow.modelData.help
          }
        }
      }
    }
  }
}
