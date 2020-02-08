import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

Item {
  id: top

  property string str: ""

  SwipeView {
    id: pageView

    clip: true
    currentIndex: pageIndicator.currentIndex
    anchors.fill: parent
    interactive: false

    SearchRoot {
      str: top.str
      onStrChanged: top.str = str;
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
}
