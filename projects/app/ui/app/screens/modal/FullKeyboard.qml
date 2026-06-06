import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/modal"
import "../../fragments/controls/name"
import "../../fragments/controls/name-full"

Page {
  id: top

  signal preClose();

  property string placeholder: "%%"
  property string str: ""
  property bool hasBox: false
  property bool is2Line: false
  property bool isPersonName: false
  property bool isPlayerName: false

  // The example demo is OWNED here (local) — it never touches the row that
  // opened the keyboard.
  function toggleExample() {
    hasBox = !hasBox;
    reUpdateExample();
  }

  function reUpdateExample() {
    if(!hasBox) {
      placeholder = "%%";
      return;
    }

    if(isPersonName && isPlayerName)
      placeholder = brg.randomExamplePlayer.randomExample();
    else if(isPersonName && !isPlayerName)
      placeholder = brg.randomExampleRival.randomExample();
    else
      placeholder = brg.randomExamplePokemon.randomExample();
  }

  onStrChanged: {
    header.str = top.str
    nameDisplay.str = str;
    pagedPicker.str = str;
  }

  // Header toolbar
  header: NameFullHeader {
    id: header

    onPreClose: top.preClose();

    chopLen: nameDisplay.chopLen
    sizeMult: nameDisplay.sizeMult
    isPersonName: top.isPersonName
    hasBox: top.hasBox

    str: top.str
    onStrChanged: top.str = str;
  }

  Pane {
    anchors.fill: parent

    PagedPicker {
      id: pagedPicker

      anchors.left: parent.left
      anchors.leftMargin: 15

      anchors.top: parent.top
      anchors.topMargin: 15

      height: parent.height - anchors.topMargin
      width: (parent.width * 0.70) - anchors.leftMargin

      str: top.str
      onStrChanged: top.str = str;
      detailView: detailView

      // Driven by the header's "View" toggle instead of swipe/dots.
      showTileset: header.showTileset
    }

    DetailView {
      id: detailView

      anchors.left: pagedPicker.right
      anchors.top: pagedPicker.top
      width: parent.width - pagedPicker.width
      height: parent.height
    }
  }

  footer: ToolBar {
    // Room for the Name/Example toggle row above the preview.
    height: ((top.hasBox) ? nameDisplay.height + 25 + (8 * 2) : 75) + 44
    Material.background: Qt.lighter(brg.settings.accentColor, 1.50)

    // Toggle the preview between just the Name and an Example sentence, with a
    // next-button to re-roll the example. Anchored (not in a layout) so the
    // NameDisplay below keeps its own width/height bindings — a layout would
    // override them and the box→name toggle would stay box-shaped/distorted.
    RowLayout {
      id: exampleControls
      anchors.top: parent.top
      anchors.topMargin: 6
      anchors.horizontalCenter: parent.horizontalCenter
      spacing: 4

      FlatToggle {
        text: top.hasBox ? "Example" : "Name"
        active: top.hasBox
        onClicked: top.toggleExample();

        MainToolTip {
          text: "Preview just the name, or the name inside an example sentence."
        }
      }

      IconButtonSquare {
        visible: top.hasBox
        Layout.alignment: Qt.AlignVCenter
        icon.width: 16
        icon.source: "qrc:/assets/icons/fontawesome/angle-double-right.svg"
        onClicked: top.reUpdateExample();

        MainToolTip { text: "Show a different random example" }
      }
    }

    NameDisplay {
      id: nameDisplay
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.top: exampleControls.bottom
      anchors.topMargin: 8

      placeholder: top.placeholder
      str: top.str
      hasBox: top.hasBox
      is2Line: top.is2Line
      isPersonName: top.isPersonName
      isPlayerName: top.isPlayerName

      disableEditor: true
      disableAutoPlaceholder: true

      centerFeedback: true
      feedbackColorNormal: "#424242"
      feedbackColorWarning: "#ef6c00"
    }
  }
}
