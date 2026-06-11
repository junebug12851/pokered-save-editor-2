// Bag.qml -- the items editor screen.
//
// Two ItemsPane side by side (a 50/50 RowLayout): the player's Bag (bound to
// player.items / brg.bagItemsModel) and the PC Storage box
// (storage.items / brg.pcItemsModel). Footer buttons: View All (slides in a
// left drawer summarising every item and where it is), Re-Roll (randomize both
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

      title: "Bag"
      box: brg.file.data.dataExpanded.player.items
      model: brg.bagItemsModel
    }

    ItemsPane {
      id: storagePane
      Layout.fillWidth: true
      Layout.fillHeight: true

      title: "Storage"
      box: brg.file.data.dataExpanded.storage.items
      model: brg.pcItemsModel
    }
  }

  // ---- "View All": a left drawer with a condensed, alphabetized table of every
  // item the user holds and how many are in the Bag vs PC Storage. -------------
  Drawer {
    id: viewAllDrawer
    edge: Qt.LeftEdge
    // Item names are short, so the table doesn't need to be wide -- the Item
    // column is fillWidth and was eating all the slack. Cap narrower (still
    // roomy enough for the longest names + both count columns).
    width: Math.min(page.width * 0.5, 360)
    height: page.height

    // Open ONLY via the footer button -- a left-edge swipe-to-open would fight the
    // bag rows' left grip handles (they sit near the window's left edge).
    dragMargin: 0

    // Refresh on every open: an amount edit via the count field writes the Item
    // directly and may not emit itemsChanged, so rebuild here to be certain the
    // table is current (the drawer is modal, so nothing changes while it's open).
    onAboutToShow: brg.itemOverviewModel.rebuild()

    // No content padding -- the default Popup padding left a white strip of the
    // drawer's background above the accent header bar. 0 makes the header flush
    // to the top (and the list flush to the edges).
    padding: 0

    Material.foreground: brg.settings.textColorDark

    ColumnLayout {
      anchors.fill: parent
      spacing: 0

      // Title bar (accent, matching the pane headers).
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

  // 3 Button Footer: View All (opens the drawer), Re-Roll, Sort.
  footer: AppFooterBtn3 {
    icon1.source: "qrc:/assets/icons/fontawesome/th.svg"
    text1: "View All"
    onBtn1Clicked: viewAllDrawer.open()

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
