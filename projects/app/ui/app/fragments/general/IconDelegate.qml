// IconDelegate.qml -- one icon-over-label tile in an IconsView grid.
//
// A flat square Button (cellSize) showing the model's icon above its capitalized
// name; clicking navigates via brg.router.changeScreen(page). Used for the Home
// menu grid. A model `disabled: true` greys the tile (slightly darkened +
// desaturated via a MultiEffect layer) and makes it non-clickable — for screens
// that aren't available yet.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

Button {
  property int cellSize: 150

  text: name
  width: cellSize
  height: cellSize

  icon.source: iconSrc
  icon.width: 75
  icon.height: 75
  icon.color: "transparent"

  font.pixelSize: 15
  font.capitalization: Font.Capitalize

  flat: true
  display: AbstractButton.TextUnderIcon
  padding: 20

  // Disabled tiles can't be clicked and have no hover/press feedback.
  enabled: !model.disabled

  // Mute disabled tiles: a slight desaturate + darken (Pokedex pattern). Tune the
  // two values to taste.
  layer.enabled: model.disabled
  layer.effect: MultiEffect {
    saturation: -0.6
    brightness: -0.15
  }

  onClicked: brg.router.changeScreen(page)
}
