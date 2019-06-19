import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Pane {
    width: 200
    anchors.left: parent.left
    height: parent.height

    ListView {
        anchors.fill: parent
        model: 25
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            width: 7
            policy: ScrollBar.AlwaysOn
        }

        delegate: ItemDelegate {
            text: qsTr("Titlezzz %1").arg(index + 1)
            width: parent.width
        }
        ScrollIndicator.vertical: ScrollIndicator {
        }
    }
}
