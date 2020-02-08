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

    InfoButton {
      x: parent.width - icon.width
      y: 0
      toolTipText: "Characters your allowed to pick in-game. These are " +
                   "guarenteed to always look the same throughout gameplay."
      icon.width: 15
      icon.height: 15

      Material.foreground: brg.settings.textColorDark
      Material.accent: brg.settings.accentColor
      toolTipEl.x: 0
    }
  }

  SearchParam {
    id: controlSearch
    text: "Control"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorControl
    Material.accent: brg.settings.fontColorControl

    InfoButton {
      x: parent.width - icon.width
      y: 0
      toolTipText: "Special codes that control the text engine. Using these " +
                   "in a name will cause glitching and trash on the screen"
      icon.width: 15
      icon.height: 15

      Material.foreground: brg.settings.textColorDark
      Material.accent: brg.settings.accentColor
      toolTipEl.x: 0
    }
  }

  SearchParam {
    id: pictureSearch
    text: "Picture"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorPicture
    Material.accent: brg.settings.fontColorPicture

    InfoButton {
      x: parent.width - icon.width
      y: 0
      toolTipText: "Tiles you can insert into the game, majority of them will " +
                   "change throughout gameplay We aproximate them here."
      icon.width: 15
      icon.height: 15

      Material.foreground: brg.settings.textColorDark
      Material.accent: brg.settings.accentColor
      toolTipEl.x: 0
    }
  }

  SearchParam {
    id: singleSearch
    text: "Single-Char"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorSingle
    Material.accent: brg.settings.fontColorSingle

    InfoButton {
      x: parent.width - icon.width
      y: 0
      toolTipText: "Characters that take up 1 space on screen and in the save " +
                   "data."
      icon.width: 15
      icon.height: 15

      Material.foreground: brg.settings.textColorDark
      Material.accent: brg.settings.accentColor
      toolTipEl.x: 0
    }
  }

  SearchParam {
    id: multiSearch
    text: "Multi-Char"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorMulti
    Material.accent: brg.settings.fontColorMulti

    InfoButton {
      x: parent.width - icon.width
      y: 0
      toolTipText: "Characters that take up more than 1 space on screen yet " +
                   "only take up 1 byte in the save data."
      icon.width: 15
      icon.height: 15

      Material.foreground: brg.settings.textColorDark
      Material.accent: brg.settings.accentColor
      toolTipEl.x: 0
    }
  }

  SearchParam {
    id: varSearch
    text: "Variable"
    onReSearch: top.reSearch();

    Material.foreground: brg.settings.fontColorVar
    Material.accent: brg.settings.fontColorVar

    InfoButton {
      x: parent.width - icon.width
      y: 0
      toolTipText: "Insert an existing name or other text from elsewhere in " +
                   "memory."
      icon.width: 15
      icon.height: 15

      Material.foreground: brg.settings.textColorDark
      Material.accent: brg.settings.accentColor
      toolTipEl.x: 0
    }
  }

  Component.onCompleted: top.reSearch();
}
