import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

GridView {
  id: view
  model: BadgesModel {}
  property int cellSize: 62

  cellWidth: cellSize
  cellHeight: cellSize
  clip: true
  interactive: false

  delegate: Image {
    width: view.cellSize
    height: view.cellSize

    MouseArea {
      anchors.fill: parent
      hoverEnabled: true
      onClicked: brg.file.data.dataExpanded.player.basics.badgeSet(index,
                                                                   !brg.file.data.dataExpanded.player.basics.badgeAt(index));

      MainToolTip {
        text: tooltipText
        visible: (followGlobalSetting)
                 ? parent.containsMouse && brg.settings.infoBtnPressed
                 : parent.containsMouse
      }
    }

    function reCalc() {
      source = brg.file.data.dataExpanded.player.basics.badgeAt(index)
                   ? iconOnSrc
                   : iconOffSrc;
      opacity = brg.file.data.dataExpanded.player.basics.badgeAt(index)
          ? 1.00
          : 0.50;
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      onBadgesChanged: reCalc();
    }

    Component.onCompleted: reCalc();
  }
}
