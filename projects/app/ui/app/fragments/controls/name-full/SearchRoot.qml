import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Row {
  id: top
  property string str: ""
  property var detailView: null

  onStrChanged: searchResults.str = str;

  SearchContainer {
    id: searchContainer

    height: parent.height
    // Wide enough for the longest labels ("Single-Char"/"Multi-Char") plus the
    // ⓘ dot and the reserved scrollbar room.
    width: 168
  }

  SearchResults {
    id: searchResults
    height: parent.height
    width: parent.width - searchContainer.width

    detailView: top.detailView
    str: top.str
    onStrChanged: top.str = str;
  }
}
