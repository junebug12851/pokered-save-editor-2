/*
  * Copyright 2026 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/**
 * Pick a character **by looking at them**.
 *
 * Twilight, 2026-07-13: *"If you need to select a picture, have a menu to select from pictures."*
 * A combo box of 72 names is a lookup table you have to already know the answer to; a grid of the
 * actual artwork is a thing you can just *see*. It is the same shape as the Blocks and Tiles pickers
 * already on this screen (BlockPick / TilePick), so it needs no learning.
 *
 * Closed, it is a button showing who is currently chosen. Open, it is the whole cast, on shelves,
 * with a filter — and the ones this map has NOT loaded carry the same yellow **!** they carry in the
 * Characters panel, because the console would draw them as garbage and that is worth knowing before
 * you pick one, not after. It is not a refusal: every one of the 256 byte values is reachable.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: pick

  /// The sprite picture id currently held (0-255).
  property int picture: 0

  signal picked(int picture)

  implicitHeight: 34

  /// Every picture in the game, plus whether THIS map has it loaded. @see MapModel::spriteCatalog
  property int revision: 0

  Connections {
    target: brg.map
    function onChanged() { pick.revision++; }
  }

  readonly property var catalog: {
    pick.revision;
    return brg.map.spriteCatalog();
  }

  readonly property var current: {
    const c = pick.catalog || [];
    for (let i = 0; i < c.length; i++)
      if (c[i].ind === pick.picture)
        return c[i];
    return null;   // a byte no sprite has. Shown as what it is -- never corrected.
  }

  // ── Closed: who is chosen ──────────────────────────────────────────────────────────────────
  Rectangle {
    id: face
    anchors.fill: parent
    radius: 4

    color: faceHover.hovered ? Qt.rgba(1, 1, 1, 0.10) : "#ffffff"
    border.width: 1
    border.color: faceHover.hovered ? "#56b4e9" : brg.settings.dividerColor

    RowLayout {
      anchors.fill: parent
      anchors.leftMargin: 5
      anchors.rightMargin: 5
      spacing: 6

      Image {
        Layout.preferredWidth: 24
        Layout.preferredHeight: 24
        visible: pick.current !== null
        source: pick.current ? pick.current.source : ""
        smooth: false
        mipmap: false
        fillMode: Image.PreserveAspectFit
      }

      Label {
        Layout.fillWidth: true
        text: pick.current
                ? pick.current.name
                : qsTr("$%1 — no character").arg(pick.picture.toString(16).toUpperCase())
        font.pixelSize: 11
        color: pick.current ? brg.settings.textColorDark : "#c79100"
        elide: Text.ElideRight
      }

      // This map hasn't loaded this one's artwork. Same mark, same meaning, as the Characters panel.
      MapWarnIcon {
        visible: pick.current !== null && pick.current.inSpriteSet === false
        text: qsTr("This map hasn't loaded this picture — the game would draw it as garbage. You "
                   + "can still choose it.")
      }

      Label {
        text: "⌄"
        font.pixelSize: 11
        opacity: 0.5
      }
    }

    HoverHandler { id: faceHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: sheet.open() }
  }

  // ── Open: the whole cast ───────────────────────────────────────────────────────────────────
  Popup {
    id: sheet

    // Hang off the panel and over the map -- the dock is 200px and a grid of faces is not.
    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    width: 380
    height: 440
    modal: true
    focus: true
    padding: 0

    background: Rectangle {
      color: "#fafafa"
      radius: 8
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    onOpened: filterField.forceActiveFocus()
    onClosed: filterField.text = ""

    property string filter: ""

    readonly property var groups: ["Story", "Trainers", "Townsfolk", "Pokemon", "Objects"]

    function matches(s) {
      return sheet.filter === "" || s.name.toLowerCase().indexOf(sheet.filter) >= 0;
    }

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: 10
      spacing: 6

      RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Label {
          Layout.fillWidth: true
          text: qsTr("Who is this?")
          font.pixelSize: 13
          font.bold: true
          color: brg.settings.textColorDark
        }

        MapRailButton {
          size: 22
          glyph: "✕"
          tip: qsTr("Close")
          onClicked: sheet.close()
        }
      }

      TextField {
        id: filterField
        Layout.fillWidth: true
        Layout.preferredHeight: 26

        placeholderText: qsTr("Filter…")
        font.pixelSize: 11
        topPadding: 0
        bottomPadding: 0
        leftPadding: 6
        rightPadding: 6

        background: Rectangle {
          radius: 4
          color: Qt.rgba(0, 0, 0, 0.04)
          border.width: 1
          border.color: filterField.activeFocus ? "#56b4e9" : brg.settings.dividerColor
        }

        onTextChanged: sheet.filter = text.trim().toLowerCase()
      }

      ScrollView {
        id: scroller
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
          // Reserve the scrollbar's lane. (The recurring gotcha -- ui-patterns.md.)
          width: scroller.availableWidth - 16
          spacing: 2

          Repeater {
            model: sheet.groups

            delegate: ColumnLayout {
              id: shelf
              required property string modelData

              Layout.fillWidth: true
              spacing: 2

              readonly property var members: (pick.catalog || []).filter(function(s) {
                return s.group === shelf.modelData && sheet.matches(s);
              })

              visible: shelf.members.length > 0

              Label {
                Layout.fillWidth: true
                Layout.topMargin: 6
                text: shelf.modelData === "Pokemon" ? qsTr("Pokémon") : shelf.modelData
                font.pixelSize: 10
                font.bold: true
                opacity: 0.45
              }

              GridLayout {
                Layout.fillWidth: true
                columns: 4
                columnSpacing: 4
                rowSpacing: 4

                Repeater {
                  model: shelf.members

                  delegate: Rectangle {
                    id: cell
                    required property var modelData

                    Layout.fillWidth: true
                    implicitHeight: 60
                    radius: 5

                    readonly property bool chosen: cell.modelData.ind === pick.picture

                    color: cell.chosen      ? Qt.rgba(0.80, 0.47, 0.65, 0.20)
                         : cellHover.hovered ? Qt.rgba(0, 0, 0, 0.06)
                         : "transparent"

                    border.width: cell.chosen ? 2 : 0
                    border.color: "#cc79a7"

                    Column {
                      anchors.centerIn: parent
                      spacing: 1

                      Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 32
                        height: 32
                        source: cell.modelData.source
                        smooth: false
                        mipmap: false
                        fillMode: Image.PreserveAspectFit
                      }

                      Label {
                        width: cell.width - 6
                        horizontalAlignment: Text.AlignHCenter
                        text: cell.modelData.name
                        font.pixelSize: 8
                        elide: Text.ElideRight
                        opacity: 0.85
                      }
                    }

                    // The same "!" as everywhere else: this map hasn't loaded that picture.
                    MapWarnIcon {
                      visible: cell.modelData.inSpriteSet === false
                      anchors.top: parent.top
                      anchors.right: parent.right
                      anchors.margins: 2
                      text: qsTr("This map hasn't loaded this picture — the game would draw it as "
                                 + "garbage. You can still choose it.")
                    }

                    HoverHandler { id: cellHover; cursorShape: Qt.PointingHandCursor }

                    TapHandler {
                      onTapped: {
                        pick.picked(cell.modelData.ind);
                        sheet.close();
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
