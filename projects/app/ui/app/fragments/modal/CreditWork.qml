// CreditWork.qml -- the faint art-attribution text pinned to a modal's
// bottom-right.
//
// A low-opacity Text the modal sets to credit its wallpaper artist (per the
// CC-BY-NC-ND license terms).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Text {
  padding: 15
  opacity: 0.40

  anchors.bottom: parent.bottom
  anchors.right: parent.right

  font.pixelSize: 13
}
