// PlaytimeEdit.qml -- the playtime "clock" row on the trainer card.
//
// Lays out the playtime sub-fields in a row -- Days : Hours : Minutes : Seconds :
// Frames -- separated by PlaytimeDividers. The fieldH and digitPad knobs keep each
// DefTextEdit compact and aligned with the rest of the card (see ui-patterns.md).
// The Frames field carries the shared overflow menu (enable/pause/randomize/clear)
// whose button shows on row hover.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"
import "../../controls/menu"

MouseArea {
  id: top

  // Shared compact field height (the sub-edits are DefTextEdits, so each one
  // takes this as its explicit height — keeps the playtime row the same height
  // as the rest of the card's fields). Defaults to the card's knob via CardFront.
  property int fieldH: 28

  hoverEnabled: true
  onContainsMouseChanged: framesEdit.menuBtnVisible = containsMouse
  width: childRow.implicitWidth
  // Pin the row to the shared field height. (Sizing to childRow.implicitHeight
  // used the Material implicit height (~48), so the 28px fields sat top-aligned
  // in a taller row and the digits + ":" rode above center.)
  height: top.fieldH

  // Each digit field only needs room for 2 characters; the Material default
  // horizontal padding made them far too wide. Trim it right down (the sub-edits'
  // width = 2*font.pixelSize + leftPadding + rightPadding, so this also narrows
  // the whole clock).
  property int digitPad: 2

  Row {
    id: childRow
    anchors.fill: parent
    spacing: 5

    DaysEdit { height: top.fieldH; leftPadding: top.digitPad; rightPadding: top.digitPad }
    PlaytimeDivider { anchors.verticalCenter: parent.verticalCenter }

    HoursEdit { height: top.fieldH; leftPadding: top.digitPad; rightPadding: top.digitPad }
    PlaytimeDivider { anchors.verticalCenter: parent.verticalCenter }

    MinutesEdit { height: top.fieldH; leftPadding: top.digitPad; rightPadding: top.digitPad }
    PlaytimeDivider { anchors.verticalCenter: parent.verticalCenter }

    SecondsEdit { height: top.fieldH; leftPadding: top.digitPad; rightPadding: top.digitPad }
    PlaytimeDivider { anchors.verticalCenter: parent.verticalCenter }

    FramesEdit {
      id: framesEdit
      height: top.fieldH
      leftPadding: top.digitPad
      rightPadding: top.digitPad
    }
  }
}
