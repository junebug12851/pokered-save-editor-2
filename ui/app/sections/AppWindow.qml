import QtQuick 2.14
import QtQuick.Controls 2.14

import "../fragments/header"

// App body is the initial modal, it allows non-modal app navigation
Page {
  id: appPage
  anchors.fill: parent

  // App-wide header
  header: AppHeader {}

  // Body
  StackView {
    id: appBody
  }
}
