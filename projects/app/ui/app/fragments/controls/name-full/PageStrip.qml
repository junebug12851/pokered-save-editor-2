// PageStrip.qml -- the keyboard's 8 page buttons.
//
// One button per modifier combination (none / Shift / Ctrl / Alt / Shift+Ctrl /
// Shift+Alt / Ctrl+Alt / Shift+Ctrl+Alt) -- which is exactly one per page, because
// 255 tiles over 36 keys needs 8 pages and three modifiers give 8 combinations.
//
// Every button shows the page's NAME and the chord that reaches it, so the strip
// teaches the shortcut instead of hiding it. Clicking one latches that chord on the
// deck, and because the deck's `page` is derived from the latches, the strip
// highlights the live page even while you're only HOLDING the keys.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
  id: strip

  property int page: 0
  signal picked(int page)

  implicitHeight: 44

  // The chips SHARE the width via the layout rather than each computing its own: all
  // eight have to fit the app's default 750px window without clipping, and a hand-
  // rolled width formula is one divide-by-zero away from a NaN -- which renders as an
  // invisible strip with no warning to tell you why. Let the layout do it.
  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 12
    spacing: 4

    Repeater {
      // pageOrder, not 0..7: a page's number is its MODIFIER MASK (Alt is page 4),
      // which is not the order a human should meet the pages in. The strip runs by
      // category, cheapest chord first.
      model: brg.keyboard.pageOrder

      Rectangle {
        id: chip

        required property var modelData
        readonly property int pageInd: modelData
        readonly property bool active: strip.page === chip.pageInd

        Layout.fillWidth: true
        Layout.maximumWidth: 130
        Layout.preferredHeight: 38
        Layout.alignment: Qt.AlignVCenter

        radius: 6

        color: active
               ? brg.settings.accentColor
               : (mouse.containsMouse
                  ? Qt.lighter(brg.settings.dividerColor, 1.12)
                  : Qt.lighter(brg.settings.dividerColor, 1.30))

        border.width: 1
        border.color: active
                      ? Qt.darker(brg.settings.accentColor, 1.2)
                      : Qt.rgba(0, 0, 0, 0.10)

        Behavior on color { ColorAnimation { duration: 100 } }

        Column {
          anchors.centerIn: parent
          width: parent.width - 6
          spacing: 0

          Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: brg.keyboard.pageNames[chip.pageInd]
            font.pixelSize: 11
            font.bold: true
            elide: Text.ElideRight
            color: chip.active ? brg.settings.textColorLight : brg.settings.textColorDark
          }

          Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            // The first page has no chord -- say so, rather than leaving a hole.
            // The badge is the SHORTCUT: the strip teaches it instead of hiding it.
            text: (chip.pageInd === 0) ? "no keys" : brg.keyboard.pageBadge(chip.pageInd)
            font.pixelSize: 8
            elide: Text.ElideRight
            color: chip.active ? brg.settings.textColorLight : brg.settings.textColorMid
            opacity: chip.active ? 0.85 : 0.8
          }
        }

        MouseArea {
          id: mouse
          anchors.fill: parent
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor
          onClicked: strip.picked(chip.pageInd);
        }
      }
    }
  }
}
