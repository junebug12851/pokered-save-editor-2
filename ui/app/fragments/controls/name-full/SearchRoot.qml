import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

Row {
  id: top
  property string str: ""

  onStrChanged: searchResults.str = str;

  SearchContainer {
    id: searchContainer

    height: parent.height
    width: 11 * 12
  }

  SearchResults {
    id: searchResults
    height: parent.height
    width: parent.width - searchContainer.width

    str: top.str
    onStrChanged: top.str = str;
  }
}
