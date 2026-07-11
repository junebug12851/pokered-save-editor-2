// FramesEdit.qml -- the "frames" field of the playtime clock.
//
// A 2-digit DefTextEdit bound to playtime.frames (clamped 0-59), dimmed when the
// clock is maxed. The last PlaytimeEdit sub-field. (The playtime overflow menu that
// used to live here was replaced by the always-visible [dice | trash] RandomButton
// on the playtime row and the [Enabled | Paused] toggle group above it -- see
// PlaytimeEdit.qml / PlaytimeToggles.qml.)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

DefTextEdit {
  id: framesEdit

  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
  maximumLength: 2
  width: 2 * font.pixelSize + leftPadding + rightPadding

  horizontalAlignment: Text.AlignHCenter

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(isNaN(txtDec))
      return;

    if(txtDec < 0 || txtDec > 59)
      return;

    brg.file.data.dataExpanded.world.other.playtime.frames = txtDec;
  }

  MainToolTip {
    text: qsTr("Frames played, 0-59")
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    function onFramesChanged() { framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames.toString(); }
  }

  Component.onCompleted: framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames.toString();
}
