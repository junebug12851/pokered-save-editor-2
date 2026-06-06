import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"

ColumnLayout {
  id: top
  spacing: 1

  signal reSearch();

  // Single-select filters. Exposed as booleans (exactly one is true).
  property alias allChecked: allSearch.checked
  property alias normalChecked: normalSearch.checked
  property alias controlChecked: controlSearch.checked
  property alias pictureChecked: pictureSearch.checked
  property alias singleChecked: singleSearch.checked
  property alias multiChecked: multiSearch.checked
  property alias varChecked: varSearch.checked

  // Makes the radios mutually exclusive even though they live in separate rows
  // (auto-exclusive only works among siblings of one parent). One stays active,
  // so there's no Clear button.
  ButtonGroup { id: filterGroup }

  // The ⓘ help affordance: shows its help ONLY while the icon itself is hovered.
  // Default ToolTip placement keeps it clear of the panel's scrollbar.
  component HelpDot: Label {
    id: dot
    property string help: ""

    text: "ⓘ"
    font.pixelSize: 15
    color: brg.settings.textColorDark
    opacity: hh.hovered ? 1.0 : 0.5
    Layout.alignment: Qt.AlignVCenter

    HoverHandler { id: hh }

    MainToolTip {
      followGlobalSetting: false
      visible: hh.hovered
      text: dot.help
    }
  }

  Spacer { Layout.preferredHeight: 2 }

  // ---- All ----
  RowLayout {
    Layout.fillWidth: true
    SearchParam {
      id: allSearch
      Layout.fillWidth: true
      text: "All"
      ButtonGroup.group: filterGroup
      onReSearch: top.reSearch();
      Material.foreground: brg.settings.textColorDark
      Material.accent: brg.settings.accentColor
    }
    HelpDot {
      help: "Show every character across all categories."
    }
  }

  Spacer { Layout.preferredHeight: 4 }

  // ---- Normal + sizes ----
  RowLayout {
    Layout.fillWidth: true
    SearchParam {
      id: normalSearch
      Layout.fillWidth: true
      // "Only" subtly signals that leaving Normal (for Single-Char, etc.) means
      // leaving the always-safe set — reinforced by its ⓘ tooltip.
      text: "Normal Only"
      checked: true
      ButtonGroup.group: filterGroup
      onReSearch: top.reSearch();
      Material.foreground: brg.settings.fontColorNormal
      Material.accent: brg.settings.fontColorNormal
    }
    HelpDot {
      help: "Characters you're allowed to pick in-game. These are guaranteed " +
            "to always look the same throughout gameplay."
    }
  }

  RowLayout {
    Layout.fillWidth: true
    SearchParam {
      id: singleSearch
      Layout.fillWidth: true
      text: "Single-Char"
      ButtonGroup.group: filterGroup
      onReSearch: top.reSearch();
      Material.foreground: brg.settings.fontColorSingle
      Material.accent: brg.settings.fontColorSingle
    }
    HelpDot {
      help: "Characters that take up 1 space on screen and in the save data."
    }
  }

  RowLayout {
    Layout.fillWidth: true
    SearchParam {
      id: multiSearch
      Layout.fillWidth: true
      text: "Multi-Char"
      ButtonGroup.group: filterGroup
      onReSearch: top.reSearch();
      Material.foreground: brg.settings.fontColorMulti
      Material.accent: brg.settings.fontColorMulti
    }
    HelpDot {
      help: "Characters that take up more than 1 space on screen yet only " +
            "take up 1 byte in the save data."
    }
  }

  RowLayout {
    Layout.fillWidth: true
    SearchParam {
      id: varSearch
      Layout.fillWidth: true
      text: "Variable"
      ButtonGroup.group: filterGroup
      onReSearch: top.reSearch();
      Material.foreground: brg.settings.fontColorVar
      Material.accent: brg.settings.fontColorVar
    }
    HelpDot {
      help: "Insert an existing name or other text from elsewhere in memory."
    }
  }

  Spacer { Layout.preferredHeight: 3 }

  // ---- Special (Picture / Control) ----
  RowLayout {
    Layout.fillWidth: true
    SearchParam {
      id: pictureSearch
      Layout.fillWidth: true
      text: "Picture"
      ButtonGroup.group: filterGroup
      onReSearch: top.reSearch();
      Material.foreground: brg.settings.fontColorPicture
      Material.accent: brg.settings.fontColorPicture
    }
    HelpDot {
      help: "Tiles you can insert into the game; the majority of them will " +
            "change throughout gameplay. We approximate them here."
    }
  }

  RowLayout {
    Layout.fillWidth: true
    SearchParam {
      id: controlSearch
      Layout.fillWidth: true
      text: "Control"
      ButtonGroup.group: filterGroup
      onReSearch: top.reSearch();
      Material.foreground: brg.settings.fontColorControl
      Material.accent: brg.settings.fontColorControl
    }
    HelpDot {
      help: "Special codes that control the text engine. Using these in a " +
            "name will cause glitching and trash on the screen."
    }
  }

  Component.onCompleted: top.reSearch();
}
