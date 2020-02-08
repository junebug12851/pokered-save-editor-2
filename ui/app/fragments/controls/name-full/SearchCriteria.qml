import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"

ColumnLayout {
  id: top
  spacing: -10

  signal reSearch();

  property alias normalSearchState: normalSearch.checkState
  property alias controlSearchState: controlSearch.checkState
  property alias pictureSearchState: pictureSearch.checkState
  property alias singleSearchState: singleSearch.checkState
  property alias multiSearchState: multiSearch.checkState
  property alias varSearchState: varSearch.checkState

  Button {
    text: "Clear"
    onClicked: {
      normalSearch.checkState = Qt.Unchecked;
      controlSearch.checkState = Qt.Unchecked;
      pictureSearch.checkState = Qt.Unchecked;
      singleSearch.checkState = Qt.Unchecked;
      multiSearch.checkState = Qt.Unchecked;
      varSearch.checkState = Qt.Unchecked;
    }
  }

  Spacer {
    height: 25
  }

  SearchParam {
    id: normalSearch
    checkState: Qt.Checked
    text: "Normal"
    onReSearch: top.reSearch();
  }

  SearchParam {
    id: controlSearch
    text: "Control"
    onReSearch: top.reSearch();
  }

  SearchParam {
    id: pictureSearch
    text: "Picture"
    onReSearch: top.reSearch();
  }

  SearchParam {
    id: singleSearch
    text: "Single-Char"
    onReSearch: top.reSearch();
  }

  SearchParam {
    id: multiSearch
    text: "Multi-Char"
    onReSearch: top.reSearch();
  }

  SearchParam {
    id: varSearch
    text: "Variable"
    onReSearch: top.reSearch();
  }

  Component.onCompleted: top.reSearch();
}
