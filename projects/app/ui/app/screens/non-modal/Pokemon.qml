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
    // columns line up exactly). The count columns are deliberately narrow -- they
    // hold a 1-2 digit number + a small number header -- so the whole table reads
    // about half as wide as a "Box 12"-labelled grid would.
    property int nameColW: 110   // wide enough for "Nidoran ♀" + the like
    property int partyColW: 46   // the Party column (fits the word)
    property int boxColW: 30     // each box column -- a 1-2 digit count + number header
    property int headerH: 32
    property int rowH: 30
    property int scrollLane: 16   // reserve the vertical scrollbar's lane on the right
    property color accent: brg.settings.accentColor

    // Width of model data column i (0 = Party, 1+ = boxes).
    function colW(i) { return (i === 0) ? partyColW : boxColW; }
    // Header text for a column: just the box number ("Box 3" -> "3"); "" for Party.
    function boxNum(label) { return (label === "Party") ? "" : label.replace("Box ", ""); }

    // Pretty-print the few species whose internal readable name carries markup,
    // mirroring the Pokedex screen's fixName() so the two screens read the same.
    function fixName(n) {
      if(n === "Nidoran<f>") return "Nidoran ♀";
      if(n === "Nidoran<m>") return "Nidoran ♂";
      if(n === "Mr.Mime")    return "Mr. Mime";
      return n;
    }

    // Zebra tints (kept very faint so a row stripe + a column band layer into a
    // clean grid rather than a busy checkerboard). Both return an alpha over white.
    function colTintAlpha(colI) { return (colI % 2 === 1) ? 0.050 : 0.0; } // Party col = 0 (untinted)
    function rowTintAlpha(rowI) { return (rowI % 2 === 1) ? 0.030 : 0.0; }

    // Columns come from the model (Party + non-empty boxes); the table's natural
    // width drives the panel width (capped so a many-box save scrolls instead of
    // overflowing the screen).
    property var cols: brg.pokemonOverviewModel.columns
    property int colCount: cols.length
    property int tableW: nameColW + partyColW + Math.max(0, colCount - 1) * boxColW + scrollLane

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

            // ---- Header row: "Species" (+ sort control) + one label per column. ----
            Item {
              width: parent.width
              height: viewAllPanel.headerH

              // Faint header bar so the titles read apart from the body.
              Rectangle {
                anchors.fill: parent
                anchors.rightMargin: viewAllPanel.scrollLane
                color: Qt.rgba(0, 0, 0, 0.05)
              }

              Row {
                anchors.fill: parent

                Item {
                  width: viewAllPanel.nameColW
                  height: parent.height

                  Text {
                    id: speciesHdr
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Species")
                    color: brg.settings.textColorMid
                    font.pixelSize: 12
                  }

                  // Sort control -- shows the icon of the CURRENT order and cycles
                  // through the same orders as the Pokedex screen on click
                  // (alphabetical / internal / dex). No tooltip; the icon itself
                  // says which order is active. The image is aspect-fit so the
                  // (non-square) source art is never squished/stretched, inside a
                  // tight square hover highlight matching the app's icon buttons.
                  Item {
                    id: sortCtl
                    anchors.left: speciesHdr.right
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    width: 26
                    height: 26

                    Rectangle {
                      anchors.fill: parent
                      radius: 2
                      color: sortMa.pressed ? Qt.rgba(0, 0, 0, 0.16)
                             : sortMa.containsMouse ? Qt.rgba(0, 0, 0, 0.08)
                             : "transparent"
                    }
                    Image {
                      anchors.centerIn: parent
                      width: 19
                      height: 19
                      source: brg.pokemonOverviewModel.sortIcon
                      fillMode: Image.PreserveAspectFit
                      sourceSize.width: 38
                      sourceSize.height: 38
                      smooth: true
                      mipmap: true
                    }
                    MouseArea {
                      id: sortMa
                      anchors.fill: parent
                      hoverEnabled: true
                      cursorShape: Qt.PointingHandCursor
                      onClicked: brg.pokemonOverviewModel.sortCycle()
                    }
                  }
                }

                Repeater {
                  model: viewAllPanel.cols
                  delegate: Item {
                    width: viewAllPanel.colW(index)
                    height: viewAllPanel.headerH
                    Text {
                      anchors.centerIn: parent
                      width: parent.width - 2
                      // Party keeps its word; boxes show just the number to stay narrow.
                      text: (index === 0) ? modelData : viewAllPanel.boxNum(modelData)
                      color: brg.settings.textColorMid
                      font.pixelSize: (index === 0) ? 11 : 13
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

            // ---- Body: column-band backdrop + one row per species. ----
            Item {
              width: parent.width
              height: Math.max(0, hFlick.height - viewAllPanel.headerH)

              // Alternate-column bands, full height (so the column colouring runs
              // past the last row too), behind the scrolling rows. The species
              // column is left untinted; the count rows draw transparent over this.
              Row {
                anchors.fill: parent
                Item { width: viewAllPanel.nameColW; height: parent.height }
                Repeater {
                  model: viewAllPanel.colCount
                  delegate: Rectangle {
                    width: viewAllPanel.colW(index)
                    height: parent.height
                    color: Qt.rgba(0, 0, 0, viewAllPanel.colTintAlpha(index))
                  }
                }
              }

              ListView {
                id: overviewList
                anchors.fill: parent
                clip: true
                model: brg.pokemonOverviewModel
                ScrollBar.vertical: ScrollBar {}

                delegate: Item {
                  id: rowRoot
                  width: overviewList.width
                  height: viewAllPanel.rowH

                  // Lift the row's roles + index onto explicit properties so the
                  // nested count Repeater can reach name/tooltip/stripe cleanly.
                  property int rowIndex: index
                  property string rowName: speciesName
                  property var rowTips: tooltips
                  property bool rowHovered: false

                  // Whole-row hover (a HoverHandler stays true over the child count
                  // cells, unlike a MouseArea's containsMouse -- same trick as the grid).
                  HoverHandler { onHoveredChanged: rowRoot.rowHovered = hovered }

                  // Alternate-row stripe (semi-transparent so the column bands still
                  // read through it), then the hover highlight on top.
                  Rectangle {
                    anchors.fill: parent
                    anchors.rightMargin: viewAllPanel.scrollLane
                    color: Qt.rgba(0, 0, 0, viewAllPanel.rowTintAlpha(rowRoot.rowIndex))
                  }
                  Rectangle {
                    anchors.fill: parent
                    anchors.rightMargin: viewAllPanel.scrollLane
                    visible: rowRoot.rowHovered
                    color: Qt.rgba(viewAllPanel.accent.r, viewAllPanel.accent.g, viewAllPanel.accent.b, 0.13)
                  }

                  Row {
                    anchors.fill: parent

                    // Species name (frozen first column), prettified.
                    Item {
                      width: viewAllPanel.nameColW
                      height: parent.height
                      Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: viewAllPanel.fixName(rowRoot.rowName)
                        color: brg.settings.textColorDark
                        font.pixelSize: 14
                        elide: Text.ElideRight
                      }
                    }

                    // One count cell per column -- 0 hidden (0 opacity), hover shows
                    // the tooltip (none when there's no tooltip text).
                    Repeater {
                      model: counts
                      delegate: Item {
                        width: viewAllPanel.colW(index)
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
