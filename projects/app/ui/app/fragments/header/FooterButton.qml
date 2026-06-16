import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls

// FooterButton.qml -- a single flat text+icon button used inside the app footers.
//
// The shared button building-block for AppFooterBtn1/2/3. Zero insets so the
// buttons tile edge-to-edge across the footer; capitalized label beside the icon.
Button {
  topInset: 0
  bottomInset: 0
  leftInset: 0
  rightInset: 0

  flat: true
  display: AbstractButton.TextBesideIcon
  icon.width: 25
  icon.height: 25
  font.capitalization: Font.Capitalize

  // Never show the hover/ripple highlight on a disabled button. A Material
  // Button that becomes disabled while the cursor is over it keeps hovered ==
  // true (Qt stops delivering hover-leave to a disabled item), leaving it stuck
  // visually highlighted -- the long-standing Checkout-button "eyesore" on the
  // Pokemart screen. Tying hoverEnabled to enabled forces hovered back to false
  // the instant the button disables, clearing the highlight. (Re-enabling
  // restores normal hovering.)
  hoverEnabled: enabled
}