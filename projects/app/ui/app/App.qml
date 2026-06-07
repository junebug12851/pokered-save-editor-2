// App.qml -- the QML application root.
//
// A StackView that hosts the whole UI: the app body (AppWindow) sits at the
// bottom of the stack and full-screen modal pages are pushed on top of it. It is
// driven by the C++ Router (brg.router): the Connections block below turns the
// Router's navigation signals into stack push/pop operations. On startup it seats
// the home + newFile screens and opens the New File modal.
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls

// Displays modals and full-screen pages
// This includes the app body itself
StackView {
  id: appRoot
  initialItem: "qrc:/ui/app/sections/AppWindow.qml"  // the always-present app body

  // Bridge the C++ Router's navigation signals to StackView operations
  Connections {
    target: brg.router

    function onGoHome() { appRoot.pop(null); }       // unwind to the app body
    function onOpenModal(url) { appRoot.push(url); }  // push a modal page
    function onCloseModal() { appRoot.pop(); }        // pop the top modal
  }

  // A file failed to load (exists but unreadable / truncated). Raise the file-error
  // screen as a full-window modal on top of wherever the user is; closing it (via
  // the router) returns them to the previous screen. The screen reads the details
  // from brg.file.lastErrorMessage / lastErrorCode.
  Connections {
    target: brg.file

    function onLoadError() { brg.router.changeScreen("fileError"); }
  }

  // Startup: seat the initial router stack, then open the New File modal
  Component.onCompleted: {
    brg.router.manualStackPush("home");
    brg.router.manualStackPush("newFile");

    appRoot.push("qrc:/ui/app/screens/modal/NewFile.qml", {}, StackView.Immediate);
  }
}
