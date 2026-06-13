// About.qml -- the "About / Credits" modal.
//
// A ListView over brg.creditsModel where each row is a CATEGORY rendered as a
// soft translucent card over the wallpaper. Each card shows a section icon +
// heading, then a Repeater over that section's entries (name, note, mandated
// attribution, clickable URL, license). A header gives the screen a title +
// warm intro; a footer shows the app version + copyright. ModalClose dismisses
// via the router; CreditWork keeps the wallpaper's mandated art attribution.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

import "../../fragments/modal"

Page {
  id: root

  // Card width is capped so lines stay readable on wide windows, and the same
  // cap is shared by the header/footer so everything lines up in one column.
  readonly property int colWidth: Math.min(width - 48, 580)

  // Maps a section heading to a (already-bundled) Font Awesome glyph. Kept in QML
  // so credits.json stays pure data; tweak freely -- it's presentation only.
  function sectionIcon(name) {
    switch(name) {
      case "Project Leaders": return "qrc:/assets/icons/fontawesome/users.svg";
      case "Data Sources":    return "qrc:/assets/icons/fontawesome/file-import.svg";
      case "Framework":       return "qrc:/assets/icons/fontawesome/cog.svg";
      case "AI Assistance":   return "qrc:/assets/icons/fontawesome/magic.svg";
      case "Tools Used":      return "qrc:/assets/icons/fontawesome/wrench.svg";
      case "Services Used":   return "qrc:/assets/icons/fontawesome/globe-americas.svg";
      case "Icons":           return "qrc:/assets/icons/fontawesome/th.svg";
      case "Wallpapers":      return "qrc:/assets/icons/fontawesome/map.svg";
      default:                return "qrc:/assets/icons/fontawesome/info-circle.svg";
    }
  }

  // URLs in the data omit the scheme; add one so the link actually opens.
  function linkHref(u) {
    return /^https?:\/\//.test(u) ? u : "https://" + u;
  }

  Image {
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent
    source: "qrc:/assets/wallpaper/starters.jpg"
    opacity: .35
  }

  ListView {
    id: creditsView
    clip: true
    model: brg.creditsModel
    anchors.fill: parent
    spacing: 18

    ScrollBar.vertical: ScrollBar {}

    // --- Title + warm intro ------------------------------------------------
    header: Item {
      width: creditsView.width
      implicitHeight: headerCol.implicitHeight + 50

      Column {
        id: headerCol
        y: 30
        width: root.colWidth
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 8

        Text {
          width: parent.width
          horizontalAlignment: Text.AlignHCenter
          text: qsTr("Credits")
          font.pixelSize: 36
          font.bold: true
          color: brg.settings.primaryColor
        }

        Text {
          width: parent.width
          horizontalAlignment: Text.AlignHCenter
          wrapMode: Text.WordWrap
          text: qsTr("A heartfelt thank-you to all the people, projects, and tools that made this editor possible.")
          font.pixelSize: 15
          color: brg.settings.textColorDark
        }
      }
    }

    // --- One translucent card per category ---------------------------------
    delegate: Item {
      width: creditsView.width
      implicitHeight: card.height

      Rectangle {
        id: card
        readonly property int pad: 22

        width: root.colWidth
        anchors.horizontalCenter: parent.horizontalCenter
        height: cardCol.implicitHeight + pad * 2
        radius: 12
        color: Qt.rgba(1, 1, 1, 0.88)
        border.width: 1
        border.color: Qt.rgba(0, 0, 0, 0.06)

        // Soft drop shadow so cards lift off the wallpaper.
        layer.enabled: true
        layer.effect: MultiEffect {
          shadowEnabled: true
          shadowColor: Qt.rgba(0, 0, 0, 0.28)
          shadowBlur: 0.5
          shadowVerticalOffset: 3
        }

        Column {
          id: cardCol
          x: card.pad
          y: card.pad
          width: card.width - card.pad * 2
          spacing: 0

          // Section heading: icon + name.
          Row {
            spacing: 12

            Item {
              width: 26
              height: 26
              anchors.verticalCenter: secTitle.verticalCenter

              Image {
                id: secIcon
                anchors.fill: parent
                source: root.sectionIcon(model.section)
                sourceSize.width: 52
                sourceSize.height: 52
                fillMode: Image.PreserveAspectFit
                visible: false
              }
              MultiEffect {
                anchors.fill: secIcon
                source: secIcon
                colorization: 1.0
                colorizationColor: brg.settings.primaryColor
              }
            }

            Text {
              id: secTitle
              text: model.section
              font.pixelSize: 22
              font.bold: true
              color: brg.settings.textColorDark
            }
          }

          Item { width: 1; height: 14 } // gap above divider

          // Divider under the heading.
          Rectangle {
            width: parent.width
            height: 1
            color: brg.settings.dividerColor
            opacity: 0.6
          }

          Item { width: 1; height: 16 } // gap below divider

          // The entries in this category.
          Repeater {
            model: entries

            delegate: Column {
              width: cardCol.width
              spacing: 3
              bottomPadding: index + 1 < entries.length ? 18 : 0

              // Credited name.
              Text {
                visible: modelData.name !== ""
                width: parent.width
                wrapMode: Text.WordWrap
                text: modelData.name
                font.pixelSize: 16
                font.bold: true
                color: brg.settings.textColorDark
              }

              // Free-text note.
              Text {
                visible: modelData.note !== ""
                width: parent.width
                wrapMode: Text.WordWrap
                text: modelData.note
                font.pixelSize: 14
                color: brg.settings.textColorMid
              }

              // Mandated attribution (license-required wording).
              Text {
                visible: modelData.mandated !== ""
                width: parent.width
                wrapMode: Text.WordWrap
                text: "“ " + modelData.mandated + " ”"
                font.pixelSize: 13
                font.italic: true
                color: brg.settings.textColorMid
              }

              // Clickable URL.
              Text {
                visible: modelData.url !== ""
                width: parent.width
                wrapMode: Text.WrapAnywhere
                textFormat: Text.StyledText
                text: '<a href="' + root.linkHref(modelData.url) + '">' + modelData.url + '</a>'
                linkColor: brg.settings.primaryColor
                font.pixelSize: 13
                font.italic: true
                onLinkActivated: (link) => Qt.openUrlExternally(link)

                // Pointer cursor over the link without swallowing the click.
                MouseArea {
                  anchors.fill: parent
                  acceptedButtons: Qt.NoButton
                  cursorShape: Qt.PointingHandCursor
                }
              }

              // License.
              Text {
                visible: modelData.license !== ""
                width: parent.width
                wrapMode: Text.WordWrap
                text: modelData.license
                font.pixelSize: 12
                font.italic: true
                color: brg.settings.textColorMid
              }
            }
          }
        }
      }
    }

    // --- App version + copyright -------------------------------------------
    footer: Item {
      width: creditsView.width
      implicitHeight: footCol.implicitHeight + 90

      Column {
        id: footCol
        y: 28
        width: root.colWidth
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 5

        Rectangle {
          width: 80
          height: 1
          color: brg.settings.dividerColor
          opacity: 0.6
          anchors.horizontalCenter: parent.horizontalCenter
        }

        Item { width: 1; height: 10 }

        Text {
          width: parent.width
          horizontalAlignment: Text.AlignHCenter
          // Show a clean "v<SemVer>" (strip any "+g<hash>" build metadata that
          // dev builds carry; the full string still lives in Qt.application.version).
          text: Qt.application.name + "  •  v" + Qt.application.version.split("+")[0]
          font.pixelSize: 13
          font.bold: true
          color: brg.settings.textColorDark
        }

        Text {
          width: parent.width
          horizontalAlignment: Text.AlignHCenter
          text: qsTr("© 2017–2026 Twilight  •  Licensed under the Apache License 2.0")
          font.pixelSize: 12
          color: brg.settings.textColorMid
        }
      }
    }
  }

  ModalClose {
    onClicked: brg.router.closeScreen();
    anchors.topMargin: 12
    anchors.rightMargin: 12
  }

  CreditWork {
    text: "\"Basic Pokemons Colors\" by yoshiyaki (CC-BY-NC-ND 3.0)\n" +
          "https://www.deviantart.com/yoshiyaki/art/Basic-Pokemons-Colors-574585879"
  }
}
