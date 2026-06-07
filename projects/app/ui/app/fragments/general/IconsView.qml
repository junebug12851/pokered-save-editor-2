// IconsView.qml -- a scrolling grid of IconDelegate tiles.
//
// A GridView (square cellSize cells) whose delegate is an IconDelegate; the caller
// supplies the model. Powers the Home screen's navigation grid.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

GridView {
  id: view
  property int cellSize: 150

  cellWidth: cellSize
  cellHeight: cellSize
  clip: true

  delegate: IconDelegate {
    cellSize: view.cellSize
  }

  ScrollBar.vertical: ScrollBar {}
}
