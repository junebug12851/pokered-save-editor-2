import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/modal"
import "../../fragments/controls/name"
import "../../fragments/controls/name-full"

Page {
  id: top

  signal toggleExample();
  signal reUpdateExample();
  signal preClose();

  property string placeholder: "%%"
  property string str: ""
  property bool hasBox: false
  property bool is2Line: false
  property bool isPersonName: false
  property bool isPlayerName: false

  onStrChanged: {
    header.str = top.str
    nameDisplay.str = str;
    pagedPicker.str = str;
  }

  // Header toolbar
  header: NameFullHeader {
    id: header

    onToggleExample: top.toggleExample();
    onReUpdateExample: top.reUpdateExample();
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
      width: (parent.width * 0.60) - anchors.leftMargin

      str: top.str
      onStrChanged: top.str = str;
    }
  }

  footer: ToolBar {
    height: (top.hasBox) ? nameDisplay.height + 25 + (8 * 2) : 75
    Material.background: Qt.lighter(brg.settings.accentColor, 1.50)

    NameDisplay {
      id: nameDisplay
      anchors.centerIn: parent

      Layout.alignment: Qt.AlignHCenter
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
      feedbackColorWarning: "#BF360C"
    }
  }
}
