// ContrastStep.qml -- a tiny flat +/- step button for an inline numeric control.
//
// Exists because a Material SpinBox does not fit in a 38px info bar: Qt 6.5+ gives its
// controls a large implicit height and it simply shoved the map screen's zoom controls off
// the footer (see ui-patterns.md -> "Material controls fight small heights"). This is the
// smallest honest thing that does the job -- a flat square with a glyph, sized by us.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
  implicitWidth: 22
  implicitHeight: 22

  topInset: 0
  bottomInset: 0
  leftInset: 0
  rightInset: 0
  padding: 0

  flat: true
  font.pixelSize: 14

  // A disabled Material button that becomes disabled under the cursor stays visually
  // highlighted (Qt stops delivering hover-leave to it). Same fix as FooterButton.
  hoverEnabled: enabled
}
