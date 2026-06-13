// Pokemon.qml -- the Pokemon storage-box editor screen.
//
// Two PokemonPane side by side (a 50/50 RowLayout), each over one of the two PC
// storage halves (brg.pokemonStorageModel1/2 with the matching
// pokemonBoxSelectModel1/2). The footer has View All (slides in a left panel
// summarising where every species lives), Re-Roll (randomizes both the storage
// boxes and the party, seeded by player.basics) and a "Boxes Setup" toggle that
// flips storage.boxesFormatted (its icon reflects the current formatted state).
// Detailed per-mon editing happens on PokemonDetails.qml, navigated to from a pane.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"
import "../../fragments/screens/pokemon"

Page {
  id: page

  // Clear all checkbox selections when this screen is actually left (popped to
  // Home or back) -- NOT when the Pokemon-detail editor is pushed over it (the
  // StackView keeps this page alive during that round-trip, so onDestruction
  // doesn't fire and the selection is preserved, which is what Twilight wants).
  // Box-switch clearing is handled in PokemonStorageModel::switchBox.
  Component.onDestruction: {
    brg.pokemonStorageModel1.clearCheckedState();
    brg.pokemonStorageModel2.clearCheckedState();
  }

  // Two equal panes via an auto layout — each fillWidth so they split the
  // screen 50/50 with no manual width math.
  RowLayout {
    anchors.fill: parent
    spacing: 0

    PokemonPane {
      id: pane1
      Layout.fillWidth: true
      Layout.fillHeight: true

      model: brg.pokemonStorageModel1
      selectModel: brg.pokemonBoxSelectModel1
    }

    PokemonPane {
      id: pane2
      Layout.fillWidth: true
      Layout.fillHeight: true

      model: brg.pokemonStorageModel2
      selectModel: brg.pokemonBoxSelectModel2
    }
  }

  // ---- "View All": a left slide-in panel with a condensed table of every
  // species the save holds and where it lives. Rows are alphabetized SPECIES
  // names (not nicknames); columns are the Party first then each NON-EMPTY box.
  // Each cell is the count in that box (0 hidden); hovering a count shows the
  // differing nicknames in it, an "...and ×N others" tail, and a caught/traded
  // split. Direct analogue of the Bag screen's View All (see Bag.qml) -- same
  // hand-rolled sliding Rectangle + scrim (a Material Drawer left a white frame),
  // same input-blocking handlers, same rebuild-on-open. ----------------------

  // Scrim behind the panel: dims the panes and closes the panel on an outside
  // click. Fades with the panel.
  Rectangle {
    id: viewAllScrim
    anchors.fill: parent
    z: 50
    color: "black"
    opacity: viewAllPanel.shown ? 0.4 : 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 180 } }

    MouseArea {
      anchors.fill: parent
      enabled: viewAllPanel.shown
      onClicked: viewAllPanel.shown = false
    }
    WheelHandler {
      enabled: viewAllPanel.shown
      onWheel: (wheel) => { wheel.accepted = true; }
    }
    HoverHandler {
      enabled: viewAllPanel.shown
      blocking: true
    }
  }

  Rectangle {
    id: viewAllPanel
    z: 51

    property bool shown: false

    // Table geometry knobs (shared by the header row and every body row so the
    // columns line up exactly).
    property int nameColW: 92
    property int countColW: 48
    property int headerH: 30
    property int rowH: 30
    property int scrollLane: 16   // reserve the vertical scrollbar's lane on the right

    // Columns come from the model (Party + non-empty boxes); the table's natural
    // width drives the panel width (capped so a many-box save scrolls instead of
    // overflowing the screen).
    property var cols: brg.pokemonOverviewModel.columns
    property int colCount: cols.length
    property int tableW: nameColW + colCount * countColW + scrollLane

    width: Math.min(page.width * 0.92, Math.max(240, tableW + 8))
    height: parent.height
    y: 0
    // Slides in from the left edge.
    x: shown ? 0 : -width
    Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    color: "white"

    // Refresh on open: species/nickname/OT edits happen in the detail editor and
    // don't emit a box pokemonChanged, so rebuild here to be sure the table is
    // current (the live connections cover add/remove/move).
    onShownChanged: if(shown) brg.pokemonOverviewModel.rebuild()

    // Absorb input so it never falls through to the panes BEHIND the panel (same
    // reasoning as Bag.qml: plain Rectangles/Text don't accept events, and the
    // bag/pokemon rows light up hover chips through a plain panel without the
    // blocking HoverHandler).
    MouseArea {
      anchors.fill: parent
      enabled: viewAllPanel.shown
    }
    WheelHandler {
      enabled: viewAllPanel.shown
      onWheel: (wheel) => { wheel.accepted = true; }
    }
    HoverHandler {
      enabled: viewAllPanel.shown
      blocking: true
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 0

      // Title bar (accent, matching the pane headers) -- flush to the very top.
      Rectangle {
        Layout.fillWidth: true
        height: 45
        color: brg.settings.accentColor

        Text {
          anchors.centerIn: parent
          text: qsTr("All Pokémon")
          color: brg.settings.textColorLight
          font.pixelSize: 18
        }
      }

      // Table area: a horizontal Flickable (so a wide, many-box table scrolls
      // sideways with a frozen-width species column) wrapping a Column of [header
      // row | vertical ListView]. The inner ListView handles vertical scroll; the
      // Flickable is horizontal-only so the two gestures don't fight.
      Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        // Empty state.
        Text {
          anchors.centerIn: parent
          visible: overviewList.count === 0
          text: qsTr("No Pokémon")
          color: brg.settings.textColorMid
          font.pixelSize: 14
        }

        Flickable {
          id: hFlick
          anchors.fill: parent
          clip: true
          contentWidth: Math.max(viewAllPanel.tableW, width)
          contentHeight: height
          flickableDirection: Flickable.HorizontalFlick
          ScrollBar.horizontal: ScrollBar {
            policy: viewAllPanel.tableW > hFlick.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
          }

          Column {
            width: Math.max(viewAllPanel.tableW, hFlick.width)

            // ---- Header row: "Species" + one label per column. ----
            Item {
              width: parent.width
              height: viewAllPanel.headerH

              Row {
                anchors.fill: parent

                Item {
                  width: viewAllPanel.nameColW
                  height: parent.height
                  Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Species")
                    color: brg.settings.textColorMid
                    font.pixelSize: 12
                  }
                }

                Repeater {
                  model: viewAllPanel.cols
                  delegate: Item {
                    width: viewAllPanel.countColW
                    height: viewAllPanel.headerH
                    Text {
                      anchors.centerIn: parent
                      width: parent.width - 4
                      text: modelData
                      color: brg.settings.textColorMid
                      font.pixelSize: 11
                      horizontalAlignment: Text.AlignHCenter
                      elide: Text.ElideRight
                    }
                  }
                }
              }

              // Divider under the titles.
              Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: viewAllPanel.scrollLane
                height: 1
                color: Qt.rgba(0, 0, 0, 0.12)
              }
            }

            // ---- Body: one row per species. ----
            ListView {
              id: overviewList
              width: parent.width
              height: Math.max(0, hFlick.height - viewAllPanel.headerH)
              clip: true
              model: brg.pokemonOverviewModel
              ScrollBar.vertical: ScrollBar {}

              delegate: Item {
                id: rowRoot
                width: overviewList.width
                height: viewAllPanel.rowH

                // Lift the row's roles onto explicit properties so the nested count
                // Repeater can reach the species name + per-column tooltip cleanly.
                property string rowName: speciesName
                property var rowTips: tooltips

                Row {
                  anchors.fill: parent

                  // Species name (frozen first column).
                  Item {
                    width: viewAllPanel.nameColW
                    height: parent.height
                    Text {
                      anchors.left: parent.left
                      anchors.leftMargin: 12
                      anchors.right: parent.right
                      anchors.rightMargin: 4
                      anchors.verticalCenter: parent.verticalCenter
                      text: rowRoot.rowName
                      color: brg.settings.textColorDark
                      font.capitalization: Font.Capitalize
                      font.pixelSize: 14
                      elide: Text.ElideRight
                    }
                  }

                  // One count cell per column -- 0 hidden (0 opacity), hover shows
                  // the tooltip.
                  Repeater {
                    model: counts
                    delegate: Item {
                      width: viewAllPanel.countColW
                      height: viewAllPanel.rowH

                      property int cnt: modelData
                      property string tip: (rowRoot.rowTips && index < rowRoot.rowTips.length)
                                           ? rowRoot.rowTips[index] : ""

                      Text {
                        anchors.centerIn: parent
                        text: cnt
                        opacity: cnt > 0 ? 1 : 0
                        color: brg.settings.textColorDark
                        font.pixelSize: 14
                      }

                      MouseArea {
                        id: cellHover
                        anchors.fill: parent
                        hoverEnabled: cnt > 0 && tip !== ""
                      }

                      ToolTip {
                        visible: cellHover.containsMouse && cnt > 0 && tip !== ""
                        text: tip
                        delay: 250
                        Material.background: brg.settings.accentColor
                        Material.foreground: brg.settings.textColorLight
                        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 80 } }
                        exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 80 } }
                      }
                    }
                  }
                }

                // Faint row separator.
                Rectangle {
                  anchors.left: parent.left
                  anchors.right: parent.right
                  anchors.bottom: parent.bottom
                  anchors.rightMargin: viewAllPanel.scrollLane
                  height: 1
                  color: Qt.rgba(0, 0, 0, 0.06)
                }
              }
            }
          }
        }
      }
    }
  }

  // 3 Button Footer: View All (slides the panel in), Re-Roll, Boxes Setup.
  footer: AppFooterBtn3 {

    icon1.source: "qrc:/assets/icons/fontawesome/th.svg"
    text1: "View All"
    onBtn1Clicked: viewAllPanel.shown = true

    // Randomize Pokemon
    icon2.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text2: "Re-Roll"
    onBtn2Clicked: {
      brg.file.data.dataExpanded.storage.randomizePokemon(brg.file.data.dataExpanded.player.basics);
      brg.file.data.dataExpanded.player.pokemon.randomize(brg.file.data.dataExpanded.player.basics);
    }

    icon3.source: (brg.file.data.dataExpanded.storage.boxesFormatted)
                  ? "qrc:/assets/icons/fontawesome/check-circle.svg"
                  : "qrc:/assets/icons/fontawesome/times-circle.svg"
    text3: "Boxes Setup"
    onBtn3Clicked: brg.file.data.dataExpanded.storage.boxesFormatted = !brg.file.data.dataExpanded.storage.boxesFormatted;
  }
}
