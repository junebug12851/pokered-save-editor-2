import QtQuick 2.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

import "../fragments/header"

// App body is the initial modal, it allows non-modal app navigation
Page {
  id: appPage
  anchors.fill: parent

  Material.background: brg.settings.textColorLight

  // App-wide header
  header: AppHeader {}

  // Body
  StackView {
    anchors.fill: parent
    id: appBody
    initialItem: "qrc:/ui/app/screens/non-modal/HomeScreen.qml"
  }
}
