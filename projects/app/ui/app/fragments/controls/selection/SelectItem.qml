// SelectItem.qml -- the item picker combo (one of the borderless Select* combos).
//
// A flat, borderless ComboBox over brg.itemSelectModel (itemSelectName/
// itemSelectInd) with a hover underline and a height-capped, scrollable popup.
// Entries with a negative index are section/disabled rows (greyed out). All the
// Select* combos in this folder share this exact pattern -- only the model, roles,
// and width differ.
//
// Duplicate guard (Bag/Items screen): set `box` (the pane's ItemStorageBox) and
// `currentItemId` (this row's current item) and the dropdown greys out / disables
// any item the box ALREADY holds -- except this row's own current item -- so the
// user can't accidentally pick a name that's already in the same pane (which keeps
// stacking tidy). Same-pane only; the other pane is irrelevant. Pre-existing
// duplicate save data is untouched -- this only blocks NEW duplicate picks. When
// `box` is null (other screens) the guard is inert and the combo behaves as before.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ComboBox {
  id: control
  textRole: "itemSelectName"
  valueRole: "itemSelectInd"

  // Duplicate guard inputs (default off -> inert on screens that don't set them).
  property var box: null        // the pane's ItemStorageBox
  property int currentItemId: -1 // this row's current item id (never greyed)

  font.capitalization: Font.Capitalize
  font.pixelSize: 14
  flat: true
  model: brg.itemSelectModel

  width: font.pixelSize * 15

  // Hover underline; defaults blue-ish (accent). Header-bar instances override
  // hoverColor to a light color to match white-text header bars.
  property color hoverColor: brg.settings.accentColor

  // Borderless at rest; a short underline on hover signals interactivity.
  background: Rectangle {
    color: "transparent"
    Rectangle {
      anchors.bottom: parent.bottom
      width: parent.width
      height: 2
      visible: control.hovered
      color: control.hoverColor
    }
  }

  delegate: ItemDelegate {
    id: itemDel
    width: control.width

    // Disabled when it's a section/header row (negative ind) OR the item is
    // already present in this pane's box and isn't this row's own current item
    // (the duplicate guard). The box check is a plain method call -- fine because
    // the popup is rebuilt each time it opens, so it re-evaluates on open.
    enabled: itemSelectInd >= 0
             && !(control.box
                  && itemSelectInd !== control.currentItemId
                  && control.box.hasItemInd(itemSelectInd))

    contentItem: Text {
      text: itemSelectName
      font: control.font
      color: itemDel.enabled
             ? brg.settings.textColorDark
             : brg.settings.textColorMid
      verticalAlignment: Text.AlignVCenter
    }

    highlighted: control.highlightedIndex === index
  }

  popup: Popup {
    y: control.height - 1
    width: control.width
    // Cap the popup so long lists scroll instead of growing to full content
    // height (at full height there is nothing to flick and it clips off-screen).
    // +2 covers the 1px padding each side. Mirrors Qt's default ComboBox popup.
    height: Math.min(contentItem.implicitHeight + 2, 280)
    padding: 1

    contentItem: ListView {
      clip: true
      implicitHeight: contentHeight
      model: control.popup.visible ? control.delegateModel : null
      currentIndex: control.highlightedIndex

      ScrollBar.vertical: ScrollBar { }
    }
  }
}
