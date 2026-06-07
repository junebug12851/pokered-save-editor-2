// ListBadges.qml -- the clickable gym-badge grid on the trainer card.
//
// A non-interactive GridView over BadgesModel; each cell shows the on/off badge
// image (dimmed when unearned) and toggles player.basics.badgeAt(index) on click.
// reCalc() refreshes from the badge bitfield and re-runs whenever badgesChanged
// fires (e.g. randomize).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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
      function onBadgesChanged() { reCalc(); }
    }

    Component.onCompleted: reCalc();
  }
}