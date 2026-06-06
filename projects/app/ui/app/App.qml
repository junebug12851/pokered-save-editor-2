import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls

// Displays modals and full-screen pages
// This includes the app body itself
StackView {
  id: appRoot
  initialItem: "qrc:/ui/app/sections/AppWindow.qml"

  Connections {
    target: brg.router

    function onGoHome() { appRoot.pop(null); }
    function onOpenModal(url) { appRoot.push(url); }
    function onCloseModal() { appRoot.pop(); }
  }

  Component.onCompleted: {
    brg.router.manualStackPush("home");
    brg.router.manualStackPush("newFile");

    appRoot.push("qrc:/ui/app/screens/modal/NewFile.qml", {}, StackView.Immediate);
  }
}
