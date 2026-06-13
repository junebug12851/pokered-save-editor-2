// Bag.qml -- the items editor screen.
//
// Two ItemsPane side by side (a 50/50 RowLayout): the player's Bag (bound to
// player.items / brg.bagItemsModel) and the PC Storage box
// (storage.items / brg.pcItemsModel). Footer buttons: View All (slides in a
// left panel summarising every item and where it is), Re-Roll (randomize both
// boxes) and Sort (sort both).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/header"
import "../../fragments/general"
import "../../fragments/screens/bag"

Page {
  id: page

  // Two equal panes via an auto layout — each fillWidth so they split the
  // screen 50/50 with no manual width math.
  RowLayout {
    anchors.fill: parent
    spacing: 0

    ItemsPane {
      id: bagPane
      Layout.fillWidth: true
      Layout.fillHeight: true

      title: qsTr("Bag")
      box: brg.file.data.dataExpanded.player.items
      model: brg.bagItemsModel
    }

    ItemsPane {
      id: storagePane
      Layout.fillWidth: true
      Layout.fillHeight: true

      title: qsTr("Storage")
      box: brg.file.data.dataExpanded.storage.items
      model: brg.pcItemsModel
    }
  }

  // ---- "View All": a left slide-in panel with a condensed, alphabetized table
  // of every item the user holds and how many are in the Bag vs PC Storage. -----
  //
  // A hand-rolled panel (not a Material Drawer): the Drawer's padding/insets/
  // elevation kept leaving a white frame around the content. This Rectangle is
  // fully under our control -- every edge is ours, so there is no stray frame.

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
    // Also swallow the wheel so the dimmed pane behind the scrim doesn't scroll.
    WheelHandler {
      enabled: viewAllPanel.shown
      onWheel: (wheel) => { wheel.accepted = true; }
    }
    // Block HOVER too, so the pane rows behind don't light up their hover chips.
    HoverHandler {
      enabled: viewAllPanel.shown
      blocking: true
    }
  }

  Rectangle {
    id: viewAllPanel
    z: 51

    property bool shown: false

    width: Math.min(page.width * 0.45, 330)
    height: parent.height
    y: 0
    // Slides in from the left edge.
    x: shown ? 0 : -width
    Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    color: "white"

    // Refresh on open: an amount edit via the count field writes the Item
    // directly and may not emit itemsChanged, so rebuild here to be sure the
    // table is current.
    onShownChanged: if(shown) brg.itemOverviewModel.rebuild()

    // Absorb input so it never falls through to the panes BEHIND the panel.
    // Plain Rectangles/Text don't accept events, so without these a click on the
    // header/empty area or a wheel the overview list doesn't consume (e.g. a
    // short list at its scroll limit) would reach the bag pane underneath. The
    // MouseArea (lowest child) catches clicks the list above doesn't; the
    // WheelHandler catches wheels the list above doesn't (the list still scrolls
    // first when it can, since it's higher in the stack). Only active while open.
    MouseArea {
      anchors.fill: parent
      enabled: viewAllPanel.shown
    }
    WheelHandler {
      enabled: viewAllPanel.shown
      onWheel: (wheel) => { wheel.accepted = true; }
    }
    // Block hover from reaching the bag rows behind (their delete chip lights up
    // on hover via a HoverHandler, which the cursor would otherwise trigger
    // through the panel). `blocking` stops items/handlers behind from hovering.
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
          text: qsTr("All Items")
          color: brg.settings.textColorLight
          font.pixelSize: 18
        }
      }

      // Column titles, aligned over the two count columns below. Right inset 20
      // reserves the list's scrollbar lane (see ui-patterns.md "Scrollable forms").
      Item {
        Layout.fillWidth: true
        height: 28

        RowLayout {
          anchors.fill: parent
          anchors.leftMargin: 15
          anchors.rightMargin: 20
          spacing: 8

          Text {
            text: qsTr("Item")
            Layout.fillWidth: true
            color: brg.settings.textColorMid
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
          }
          Text {
            text: qsTr("Bag")
            Layout.preferredWidth: 56
            horizontalAlignment: Text.AlignRight
            color: brg.settings.textColorMid
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
          }
          Text {
            text: qsTr("Storage")
            Layout.preferredWidth: 66
            horizontalAlignment: Text.AlignRight
            color: brg.settings.textColorMid
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
          }
        }

        // Divider under the titles.
        Rectangle {
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.bottom: parent.bottom
          height: 1
          color: Qt.rgba(0, 0, 0, 0.12)
        }
      }

      ListView {
        id: overviewList
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: brg.itemOverviewModel
        ScrollBar.vertical: ScrollBar {}

        // Empty state.
        Text {
          anchors.centerIn: parent
          visible: overviewList.count === 0
          text: qsTr("No items")
          color: brg.settings.textColorMid
          font.pixelSize: 14
        }

        delegate: Item {
          width: ListView.view ? ListView.view.width : 0
          height: 30

          RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 20      // reserve the scrollbar lane
            spacing: 8

            Text {
              text: itemName
              Layout.fillWidth: true
              color: brg.settings.textColorDark
              font.capitalization: Font.Capitalize
              font.pixelSize: 14
              elide: Text.ElideRight
              verticalAlignment: Text.AlignVCenter
            }

            // Bag count -- hidden (0 opacity) when none in the bag.
            Text {
              text: bagCount
              opacity: bagCount > 0 ? 1 : 0
              Layout.preferredWidth: 56
              horizontalAlignment: Text.AlignRight
              color: brg.settings.textColorDark
              font.pixelSize: 14
              verticalAlignment: Text.AlignVCenter
            }

            // Storage count -- hidden (0 opacity) when none in storage.
            Text {
              text: storageCount
              opacity: storageCount > 0 ? 1 : 0
              Layout.preferredWidth: 66
              horizontalAlignment: Text.AlignRight
              color: brg.settings.textColorDark
              font.pixelSize: 14
              verticalAlignment: Text.AlignVCenter
            }
          }

          // Faint row separator.
          Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Qt.rgba(0, 0, 0, 0.06)
          }
        }
      }
    }
  }

  // 3 Button Footer: View All (slides the panel in), Re-Roll, Sort.
  footer: AppFooterBtn3 {
    icon1.source: "qrc:/assets/icons/fontawesome/th.svg"
    text1: "View All"
    onBtn1Clicked: viewAllPanel.shown = true

    icon2.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text2: "Re-Roll"
    onBtn2Clicked: {
      brg.file.data.dataExpanded.player.items.randomize()
      brg.file.data.dataExpanded.storage.items.randomize()
    }

    icon3.source: "qrc:/assets/icons/fontawesome/sort-amount-up.svg"
    text3: "Sort"
    onBtn3Clicked: {
      brg.file.data.dataExpanded.player.items.sort();
      brg.file.data.dataExpanded.storage.items.sort();
    }
  }
}
