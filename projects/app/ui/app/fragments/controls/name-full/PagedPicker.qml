// PagedPicker.qml -- the left pane of the full keyboard (character picker).
//
// A non-interactive SwipeView over two pages selected by showTileset (driven by
// the header's View toggle): the SearchRoot character list (page 0) and the
// TilesetPicker grid (page 1). Both edit the shared str and feed a DetailView.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
  id: top

  property string str: ""
  property var detailView: null

  // false = character list, true = simulated tileset grid. Driven by the
  // header's "View" toggle (no more swipe gesture / page dots, which clipped
  // over the content and were unintuitive).
  property bool showTileset: false

  onStrChanged: {
    searchRoot.str = str;
    tilesetPicker.str = str;
  }

  SwipeView {
    id: pageView

    clip: true
    currentIndex: top.showTileset ? 1 : 0
    anchors.fill: parent
    interactive: false

    SearchRoot {
      id: searchRoot
      str: top.str
      onStrChanged: top.str = str;
      detailView: top.detailView
    }

    TilesetPicker {
      id: tilesetPicker
      str: top.str
      onStrChanged: top.str = str;
      detailView: top.detailView
    }
  }
}
