import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/modal"
import "../../common/Style.js" as Style

Rectangle {
  id: top

  signal close();

  color: Style.primaryColorLight

  width: parent.width
  height: parent.height

  ModalClose {
    onClicked: exitAnim.start()
  }

  // Entrace animation
  NumberAnimation on y {
    from: -1000
    to: 0
    duration: 250
  }

  NumberAnimation on opacity {
    from: 0
    to: 1
    duration: 100
  }

  // Exit animation
  ParallelAnimation {
    id: exitAnim
    NumberAnimation {target: top; property: "y"; to: -1000; duration: 250}
    NumberAnimation {target: top; property: "opacity"; to: 0; duration: 250}

    onFinished: top.close();
  }
}
