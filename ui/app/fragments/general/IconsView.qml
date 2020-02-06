import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

GridView {
  id: view
  property int cellSize: 150

  cellWidth: cellSize
  cellHeight: cellSize
  clip: true

  delegate: IconDelegate {
    cellSize: view.cellSize
  }
}
