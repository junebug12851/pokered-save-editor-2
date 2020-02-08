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

    Material.foreground: brg.settings.fontColorNormal
    Material.accent: brg.settings.fontColorNormal
  }

  SearchParam {
    id: controlSearch
    text: "Control"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorControl
    Material.accent: brg.settings.fontColorControl
  }

  SearchParam {
    id: pictureSearch
    text: "Picture"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorPicture
    Material.accent: brg.settings.fontColorPicture
  }

  SearchParam {
    id: singleSearch
    text: "Single-Char"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorSingle
    Material.accent: brg.settings.fontColorSingle
  }

  SearchParam {
    id: multiSearch
    text: "Multi-Char"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorMulti
    Material.accent: brg.settings.fontColorMulti
  }

  SearchParam {
    id: varSearch
    text: "Variable"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorVar
    Material.accent: brg.settings.fontColorVar
  }

  Component.onCompleted: top.reSearch();
}
