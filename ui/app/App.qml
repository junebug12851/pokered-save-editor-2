import QtQuick 2.14
import QtQuick.Controls 2.14

// Displays modals and full-screen pages
// This includes the app body itself
StackView {
  id: appRoot
  initialItem: "qrc:/ui/app/sections/AppWindow.qml"
}
