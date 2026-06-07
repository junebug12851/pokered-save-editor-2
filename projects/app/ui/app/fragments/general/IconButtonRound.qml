// IconButtonRound.qml -- a minimal flat, icon-only round button.
//
// A small reusable round icon button (15x15 icon); callers set icon.source and
// onClicked.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

RoundButton {
  display: AbstractButton.IconOnly
  icon.width: 15
  icon.height: 15
  flat: true
}
