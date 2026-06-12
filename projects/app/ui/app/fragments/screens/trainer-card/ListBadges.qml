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

    // The badge icons and gym-leader shadows have varying, non-square dimensions
    // (badges ~square, leader silhouettes tall & narrow). Fit each within the cell
    // preserving its own aspect ratio so nothing is stretched; PreserveAspectFit
    // also centers the painted image inside the square cell. mipmap + a capped
    // sourceSize keep the large source art crisp and memory-light at this small size.
    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: true
    sourceSize.width: view.cellSize * 4
    sourceSize.height: view.cellSize * 4

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
      // Unearned badges now show the gym leader's shadow at full opacity (no
      // dimming) -- the shadow image itself conveys the not-yet-earned state.
      opacity = 1.00;
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      function onBadgesChanged() { reCalc(); }
    }

    Component.onCompleted: reCalc();
  }
}