// ConfirmPopup.qml -- a reusable "informed consent" confirmation modal.
//
// Centered, dimmed modal with an alert header (info icon + title), a body
// message, and Cancel / Confirm actions. Destructive operations (reset, max out,
// validate, evolve/de-evolve, delete/remove, whole-mon re-roll, ...) gate behind
// this so the user knows what's about to happen. Call ask(...) with the action to
// run on confirm:
//
//   confirm.ask("Reset Pokémon?", "Resets to a fresh level-5…",
//               function(){ boxData.resetPokemon(); }, "Reset");
//
// Matches the storage "Boxes Formatted" warn modal's look (Pokemon.qml).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

Popup {
  id: pop

  property string titleText: ""
  property string bodyText: ""
  property string confirmText: qsTr("Confirm")
  // The action to run when the user confirms (set via ask()).
  property var acceptAction: null

  function ask(title, body, accept, confirmLabel) {
    titleText = title;
    bodyText = body;
    acceptAction = accept;
    confirmText = (confirmLabel && confirmLabel.length > 0) ? confirmLabel : qsTr("Confirm");
    open();
  }

  parent: Overlay.overlay
  anchors.centerIn: Overlay.overlay
  modal: true
  dim: true
  focus: true
  closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
  padding: 0
  width: Math.min((Overlay.overlay ? Overlay.overlay.width : 480) - 80, 440)

  background: Rectangle {
    color: brg.settings.textColorLight
    radius: 10
    border.color: Qt.rgba(0, 0, 0, 0.15)
    border.width: 1
  }

  contentItem: ColumnLayout {
    spacing: 0

    // Alert header bar.
    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: 52
      color: brg.settings.primaryColor
      radius: 10
      // Square off the bottom corners so the bar meets the body cleanly.
      Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 12
        color: parent.color
      }

      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 10

        Image {
          source: "qrc:/assets/icons/fontawesome/info-circle.svg"
          sourceSize.width: 24
          sourceSize.height: 24
          Layout.preferredWidth: 24
          Layout.preferredHeight: 24
          layer.enabled: true
          layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: brg.settings.textColorLight
          }
        }
        Text {
          Layout.fillWidth: true
          text: pop.titleText
          color: brg.settings.textColorLight
          font.pixelSize: 18
          font.bold: true
        }
      }
    }

    // Body message.
    Text {
      Layout.fillWidth: true
      Layout.margins: 18
      wrapMode: Text.WordWrap
      lineHeight: 1.25
      color: brg.settings.textColorDark
      font.pixelSize: 14
      text: pop.bodyText
    }

    // Actions.
    RowLayout {
      Layout.fillWidth: true
      Layout.leftMargin: 16
      Layout.rightMargin: 16
      Layout.bottomMargin: 14
      spacing: 8

      Item { Layout.fillWidth: true }

      Button {
        flat: true
        text: qsTr("Cancel")
        onClicked: pop.close()
      }
      Button {
        highlighted: true
        Material.background: brg.settings.primaryColor
        Material.foreground: brg.settings.textColorLight
        text: pop.confirmText
        onClicked: {
          if(pop.acceptAction)
            pop.acceptAction();
          pop.close();
        }
      }
    }
  }
}
