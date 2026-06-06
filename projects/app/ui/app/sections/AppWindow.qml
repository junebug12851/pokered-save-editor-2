import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls

import "../fragments/header"

// App body is the initial modal, it allows non-modal app navigation
Page {
  id: appPage

  Material.background: brg.settings.textColorLight

  // App-wide header
  header: AppHeader {}

  // Body
  StackView {
    anchors.fill: parent
    id: appBody
    initialItem: "qrc:/ui/app/screens/non-modal/Home.qml"

    Connections {
      target: brg.router

      function onGoHome() { appBody.pop(null); }
      function onOpenNonModal(url) { appBody.push(url); }
      function onCloseNonModal(url) { appBody.pop(url); }
    }
  }
}
