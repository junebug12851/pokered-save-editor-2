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

  SwipeView {
    id: pageView

    clip: true
    currentIndex: pageIndicator.currentIndex
    anchors.left: parent.left
    anchors.leftMargin: 15

    anchors.top: parent.top
    anchors.topMargin: 15

    height: parent.height - 15
    width: (parent.width * 0.60) - 15

    interactive: false

    Row {
      SearchContainer {
        id: searchContainer

        height: parent.height
        width: 11 * 12
      }

      ListView {
        id: listView

        height: parent.height
        width: parent.width - searchContainer.width

        clip: true
        model: brg.fontSearchModel
        ScrollBar.vertical: ScrollBar {}

        property alias str: top.str

        delegate: Button {
          width: listView.width
          flat: true
          text: (brg.fonts.fontAt(fontInd).alias !== "")
                ? brg.fonts.fontAt(fontInd).alias
                : brg.fonts.fontAt(fontInd).name
          font.capitalization: Font.MixedCase

          hoverEnabled: true
          onClicked: {
            var fontName = brg.fonts.fontAt(fontInd).name;
            var curStr = listView.str.toString();

            listView.str = curStr + fontName;
          }
        }
      }
    }

    Rectangle {
      color: "red"
    }
  }

  PageIndicator {
    id: pageIndicator

    interactive: true
    count: pageView.count
    currentIndex: pageView.currentIndex

    anchors.bottom: pageView.bottom
    anchors.horizontalCenter: pageView.horizontalCenter

    delegate: Rectangle {
      implicitWidth: 12
      implicitHeight: 12

      radius: width / 2
      color: brg.settings.textColorDark

      opacity: (index === pageIndicator.currentIndex)
               ? 0.95
               : 0.45

      Behavior on opacity {
        OpacityAnimator {
          duration: 100
        }
      }
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
