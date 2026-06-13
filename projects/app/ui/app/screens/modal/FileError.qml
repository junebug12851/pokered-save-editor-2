// FileError.qml -- the full-window "couldn't load this file" modal.
//
// Raised by App.qml when brg.file emits loadError() -- i.e. a file that exists
// couldn't be opened (in use / no permission) or is truncated/corrupted. The
// plain-English reason (brg.file.lastErrorMessage) is the centred primary message;
// the real one-line technical detail (brg.file.lastErrorDetail -- the OS/Qt error
// string or size mismatch) sits small and muted at the bottom. ModalClose dismisses
// via the router, returning to the previous screen (same as the other modals).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

import "../../fragments/modal"

Page {
  id: top

  Material.background: brg.settings.textColorLight // themed background

  // Centered message block
  ColumnLayout {
    anchors.centerIn: parent
    width: Math.min(parent.width - 96, 520)
    spacing: 18

    Image {
      source: "qrc:/assets/icons/fontawesome/info-circle.svg"
      sourceSize.width: 56
      sourceSize.height: 56
      Layout.alignment: Qt.AlignHCenter
      // Tint the glyph with the primary (alert) colour (MultiEffect, as used in Pokedex.qml)
      layer.enabled: true
      layer.effect: MultiEffect {
        colorization: 1.0
        colorizationColor: brg.settings.primaryColor
      }
    }

    Text {
      text: qsTr("Couldn't open this file")
      color: brg.settings.textColorDark
      font.pixelSize: 24
      font.bold: true
      horizontalAlignment: Text.AlignHCenter
      Layout.alignment: Qt.AlignHCenter
      Layout.fillWidth: true
      wrapMode: Text.WordWrap
    }

    Text {
      text: brg.file.lastErrorMessage
      color: Qt.lighter(brg.settings.textColorDark, 1.35)
      font.pixelSize: 15
      lineHeight: 1.15
      horizontalAlignment: Text.AlignHCenter
      Layout.alignment: Qt.AlignHCenter
      Layout.fillWidth: true
      wrapMode: Text.WordWrap
    }
  }

  // The real technical detail (OS/Qt error string or size mismatch), small and
  // muted along the bottom -- secondary to the plain message, never the headline.
  Text {
    visible: brg.file.lastErrorDetail !== ""
    text: brg.file.lastErrorDetail
    color: brg.settings.textColorMid
    opacity: 0.7
    font.pixelSize: 12
    font.italic: true
    horizontalAlignment: Text.AlignHCenter
    wrapMode: Text.WordWrap
    elide: Text.ElideRight

    width: Math.min(parent.width - 48, 600)
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 16
  }

  ModalClose {
    onClicked: brg.router.closeScreen();
  }
}
