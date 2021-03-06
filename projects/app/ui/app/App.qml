import QtQuick 2.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

// Displays modals and full-screen pages
// This includes the app body itself
StackView {
  id: appRoot
  initialItem: "qrc:/ui/app/sections/AppWindow.qml"

  Connections {
    target: brg.router

    onGoHome: appRoot.pop(null);
    onOpenModal: appRoot.push(url);
    onCloseModal: {
      appRoot.pop();
    }
  }

  Component.onCompleted: {
    brg.router.manualStackPush("home");
    brg.router.manualStackPush("newFile");

    appRoot.push("qrc:/ui/app/screens/modal/NewFile.qml", {}, StackView.Immediate);
  }
}
