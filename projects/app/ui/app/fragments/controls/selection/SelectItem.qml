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
// When `box` is set the dropdown also appends each item's TOTAL owned across both
// panes (bag + storage) in parens, e.g. "POTION  (x12)", so the user can see what
// they already hold elsewhere at a glance.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"

ComboBox {
  id: control
  textRole: "itemSelectName"
  valueRole: "itemSelectInd"

  // Detailed (help-mode) tooltip for the currently-selected item: what it is and
  // how it's obtained. Tied to the item, so it shows wherever this picker is used
  // (the Bag/Items screen and anywhere else), gated on the header "?" toggle.
  DetailToolTip {
    title: control.currentText
    text: brg.itemSelectModel.infoForInd(control.currentValue)
    hovered: control.hovered
  }

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
      // Item name, plus the TOTAL owned across BOTH panes (bag + storage) in
      // parens, e.g. "POTION  (x12)". This lets the user see they already own
      // some in the other pane even when this pane has none -- the dropdown is
      // alphabetized/categorized, so it's a reliable quick glance regardless of
      // whether either box is sorted. Hidden for section rows and when the total
      // is 0 (don't clutter items the user doesn't own anywhere).
      text: {
        if(itemSelectInd < 0 || !control.box)
          return itemSelectName;
        var total = control.box.amountOfInd(itemSelectInd)
                    + control.box.destBox.amountOfInd(itemSelectInd);
        return (total > 0)
            ? itemSelectName + "  (x" + total + ")"
            : itemSelectName;
      }
      font: control.font
      color: itemDel.enabled
             ? brg.settings.textColorDark
             : brg.settings.textColorMid
      verticalAlignment: Text.AlignVCenter
    }

    highlighted: control.highlightedIndex === index

    // Per-item detailed tooltip while browsing the dropdown (section rows have no
    // info, so they stay quiet).
    DetailToolTip {
      title: itemSelectName
      text: itemSelectInfo
      hovered: itemDel.hovered
    }
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
