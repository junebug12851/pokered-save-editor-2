import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

Row {
  id: top
  property string str: ""

  SearchContainer {
    id: searchContainer

    height: parent.height
    width: 11 * 12
  }

  SearchResults {
    height: parent.height
    width: parent.width - searchContainer.width

    str: top.str
    onStrChanged: top.str = str;
  }
}
