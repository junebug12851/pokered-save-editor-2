// ItemBoxView.qml -- the scrolling list of item rows inside an ItemsPane.
//
// A ListView over an ItemStorageBox. Each real row is a left grip handle + a
// checkbox + a SelectItem combo + a 2-digit count field (0-99) + a hover/checked
// delete chip; the trailing placeholder row shows a "+" add button while there's
// room. The rowH/comboH/textH knobs pin the differing Material control heights so
// the row aligns (see ui-patterns.md). Twilight's inline notes flag the custom
// itemToListIndex lookup (far faster than Qt's indexOfValue) and the insert
// "hack" -- leave them.
//
// Drag & drop (the list analogue of PokemonBoxView's grid drag): each row is a
// DropArea; its content is a drag target that reparents to the window overlay
// while dragging (so the ghost floats across both panes). Because a row holds
// interactive controls (combo / count field), the drag is started ONLY from the
// left grip handle, never the whole row. Dropping reorders within the list
// (dragReorder) or transfers in from the other pane (dragTransfer); the drop slot
// is the targeted row (insert before it; the trailing "+" row appends). If the
// grabbed item is checked, the whole checked set moves (group drag). A dashed
// horizontal caret marks the hovered drop slot. See notes/reference/ui-patterns.md.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.ItemStorageBox

import "../../general"
import "../../controls/selection"

ListView {
  id: itemBoxView

  property ItemStorageBox box: null

  // Per-control height knobs (Qt 6 Material controls each have a different
  // implicit height; pin them so the checkbox / combo / count field share one
  // consistent, vcentered row — see notes/reference/ui-patterns.md).
  property int rowH: 38     // the row's height (the combo sets the tallest)
  property int comboH: 38   // SelectItem dropdown
  property int textH: 30    // count text box (a touch shorter)

  // Floating layer for the drag "ghost": the window overlay, so a row being
  // dragged stays visible across BOTH panes and isn't clipped by this ListView.
  property Item dragLayer: Overlay.overlay

  clip: true
  ScrollBar.vertical: ScrollBar {}

  // Breathing room below the last row (replaces an old empty-Text spacer).
  footer: Item { width: 1; height: 25 }

  // Hack because insert doesn't work as exoected
  function hack_newAndRePositionViewEnd() {
    box.itemNew();
    positionViewAtEnd();
  }

  // Each row is a DropArea so an item can be dropped onto it (drop-to-commit, no
  // live reshuffle). The visible content lives in a child `content` Item that
  // becomes the drag target; while dragging it reparents to the overlay so the
  // ghost floats across both panes. The drag is driven only by the grip handle
  // (the rest of the row stays interactive). Group drags (the grabbed item is
  // checked) carry the whole checked set, handled by dragReorder/dragTransfer.
  delegate: DropArea {
    id: row
    width: ListView.view ? ListView.view.width : 0
    height: itemBoxView.rowH

    // --- Info the drop target reads off the dragged item (drop.source) ---
    property var ownerModel: itemBoxView.model
    property int rowIndex: index
    property bool grabbedChecked: (itemChecked === true)
    property bool isPlaceholder: (itemIsPlaceholder === true)

    // An item was dropped onto this slot: reorder within the list, or transfer in
    // from the other pane. toIndex == this slot (insert before it); the trailing
    // placeholder row's index == count, so dropping there appends.
    onDropped: (drop) => {
      var s = drop.source;
      if(s === null || s === undefined || s.isPlaceholder === true)
        return;

      var srcModel = s.ownerModel;
      var srcIndex = s.rowIndex;
      var grp = (s.grabbedChecked === true);
      var toIndex = row.rowIndex;
      var samePane = (srcModel === itemBoxView.model);

      // Defer the model mutation: the drop resets the model (rebuilding these
      // delegates), but the dragged content is still parented to the overlay
      // until the release settles. Running it next tick lets the ghost return to
      // its row first, so no delegate is destroyed mid-drag (no orphaned ghost).
      Qt.callLater(function() {
        if(samePane)
          srcModel.dragReorder(srcIndex, toIndex, grp);
        else
          srcModel.dragTransfer(srcIndex, toIndex, grp);
      });
    }

    // Insertion caret: a dashed HORIZONTAL bar straddling this row's TOP edge
    // (the gap *before* it), shown while a drag hovers this slot. It's an overlay
    // -- it never resizes or shuffles the rows. The drop inserts before this row;
    // hovering the trailing "+" row marks "after the last item" (its top edge is
    // the gap between the last item and the New button).
    Canvas {
      id: dropHint
      height: 6
      width: parent.width - 20
      anchors.top: parent.top
      anchors.topMargin: -3             // center the bar on the row boundary
      anchors.horizontalCenter: parent.horizontalCenter
      z: 300
      visible: row.containsDrag
      onVisibleChanged: requestPaint()
      onWidthChanged: requestPaint()
      onPaint: {
        var ctx = getContext("2d");
        ctx.reset();
        ctx.clearRect(0, 0, width, height);
        ctx.strokeStyle = brg.settings.accentColor;
        ctx.lineWidth = 3;
        ctx.lineCap = "round";
        ctx.setLineDash([5, 4]);
        ctx.beginPath();
        ctx.moveTo(2, height / 2);
        ctx.lineTo(width - 2, height / 2);
        ctx.stroke();
      }
    }

    // The visible row content. Fills the row at rest; while dragging it reparents
    // to itemBoxView.dragLayer (the overlay) and its anchors clear so the drag
    // handler can move it freely.
    Item {
      id: content
      width: row.width
      height: row.height
      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter

      Drag.source: row
      // Grab point near the grip (where the cursor actually is), so the ghost
      // doesn't jump out from under the pointer on lift.
      Drag.hotSpot.x: 24
      Drag.hotSpot.y: height / 2

      // Drive the Drag manually. An internal MouseArea drag never auto-commits,
      // so DropArea.onDropped won't fire on its own -- we have to call Drag.drop()
      // ourselves on release. We hold Drag.active true from when the grip drag
      // begins until we've dropped, so the drop lands on the DropArea under the
      // cursor; setting it false then reverts the lift state (reparent back).
      property bool maActive: dragHandler.drag.active
      onMaActiveChanged: {
        if(maActive) {
          content.Drag.active = true;
        }
        else if(content.Drag.active) {
          content.Drag.drop();      // -> fires DropArea.onDropped under the cursor
          content.Drag.active = false;
        }
      }

      // Whole-row hover. Driving the delete chip's visibility off a HoverHandler
      // (not a MouseArea) keeps it shown when the pointer moves onto it -- a
      // hovered Button would otherwise steal hover and hide the very button you're
      // reaching for.
      HoverHandler { id: cellHover }

      // While being dragged: a clean "card" so the floating row reads as lifted.
      Rectangle {
        anchors.fill: parent
        visible: content.Drag.active
        color: "white"
        radius: 4
        border.color: brg.settings.accentColor
        border.width: 1
        z: -1
      }

      RowLayout {
        id: rowEntry
        // Left-aligned so each row's checkbox forms a column directly under the
        // header's check-all button (both share the list's 15px left inset; the
        // header check-all is nudged right to clear the grip handle). Spans to the
        // right with a 16px margin to RESERVE THE SCROLLBAR LANE -- the Material
        // ScrollBar is a right-edge overlay, so the trailing delete button would
        // otherwise sit under it and be unclickable (recurring gotcha; see
        // ui-patterns.md "Scrollable forms"). The combo is the fillWidth element,
        // so when space is tight it shrinks to keep the delete clear, and on wide
        // panes it just caps at its normal width (no visible change).
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        visible: !itemIsPlaceholder

        // Grip handle -- the ONLY drag initiator (the rest of the row stays
        // clickable/typable). A muted set of lines that darkens on hover; an
        // open-hand cursor signals "drag me". The drag.target is `content`, so
        // grabbing the grip lifts the whole row.
        Item {
          id: grip
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredWidth: 24
          Layout.preferredHeight: itemBoxView.rowH

          Image {
            anchors.centerIn: parent
            width: 18
            height: 18
            sourceSize.width: width
            sourceSize.height: height
            source: "qrc:/assets/icons/fontawesome/grip-lines.svg"
            fillMode: Image.PreserveAspectFit
            opacity: dragHandler.containsMouse ? 0.8 : 0.4
            Behavior on opacity { NumberAnimation { duration: 90 } }
          }

          MouseArea {
            id: dragHandler
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.OpenHandCursor
            preventStealing: true            // keep the ListView flick from stealing the drag
            drag.target: row.isPlaceholder ? null : content
            drag.threshold: 8                // a press that moves less than this stays a click
          }
        }

        CheckBox {
          id: selectBox

          // VCenter + minimumHeight:0 so the Material checkbox's ~40px touch
          // target doesn't floor the row or ride high (ui-patterns.md →
          // "Material controls fight small heights").
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredHeight: itemBoxView.rowH
          Layout.minimumHeight: 0

          // Bind to the model role (persists across model resets, since delegates
          // are recreated with a fresh binding); write back only on a user toggle.
          // Mirrors PokemonBoxView's robust pattern -- a one-shot
          // Component.onCompleted didn't restore on delegate reuse, so checks
          // "disappeared" after a drag reset.
          checked: (itemChecked === true)
          onToggled: itemChecked = checked
        }

        SelectItem {
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredHeight: itemBoxView.comboH
          // The flexible element: caps at its normal width, but shrinks when the
          // row is tight so the trailing delete button stays clear of the
          // reserved scrollbar lane (see rowEntry's rightMargin).
          Layout.fillWidth: true
          Layout.maximumWidth: font.pixelSize * 15
          Layout.minimumWidth: font.pixelSize * 7

          // Duplicate guard: grey out item names already present in THIS pane's
          // box (except this row's own current item) so the user can't make an
          // accidental duplicate. Same pane only.
          box: itemBoxView.box
          currentItemId: itemId

          onActivated: itemId = currentValue;
          Component.onCompleted: {

            // WAAAAYYYY FASTER
            // I have no idea why the native function to do this is a million times
            // slower. It's a billion times faster to implement my own in C++
            // and I'm not even dealing with that many rows.
            currentIndex = brg.itemSelectModel.itemToListIndex(itemId);

            // EXTREMELY SLOW
            //currentIndex = indexOfValue(itemId);
          }
        }

        DefTextEdit {
          id: countEdit

          // VCenter + pin the field height so it lines up with the combo.
          // DefTextEdit already centers its text at any height (topPadding:0).
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredHeight: itemBoxView.textH
          Layout.preferredWidth: 2 * font.pixelSize + leftPadding + rightPadding

          maximumLength: 2

          onTextChanged: {
            if(text === "")
              return;

            var dec = parseInt(text, 10);
            if(isNaN(dec))
              return;

            if(dec < 0 || dec > 99)
              return;

            itemCount = dec;
          }

          Component.onCompleted: (itemCount !== undefined)
                                 ? text = itemCount
                                 : text = "";
        }

        // Delete chip, just to the RIGHT of the count field, shown on hover OR
        // when checked (Twilight's placement). Same chip as the Pokemon grid's
        // per-cell delete: an opaque accent circle with a white "times" glyph at
        // rest, fills red on hover, darkens on press. A checked item deletes the
        // whole checked set (deleteItem group -> checkedDelete); otherwise just
        // this item.
        Button {
          id: deleteBtn
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredWidth: 28
          Layout.preferredHeight: 28

          // Reserve the slot ALWAYS (it stays in the layout) and fade it with
          // opacity rather than `visible` -- a `visible:false` layout item
          // COLLAPSES to zero width, which reflowed the whole row (the fillWidth
          // combo grabbed/released the space) every time you hovered. `enabled`
          // tracks the same condition so the invisible chip can't be clicked.
          property bool shown: cellHover.hovered || (itemChecked === true)
          opacity: shown ? 1 : 0
          enabled: shown
          Behavior on opacity { NumberAnimation { duration: 90 } }

          padding: 0
          topInset: 0
          bottomInset: 0
          leftInset: 0
          rightInset: 0
          display: AbstractButton.IconOnly

          icon.source: "qrc:/assets/icons/fontawesome/times.svg"
          // The icon is STRETCHED to fill icon.width x icon.height, so use the
          // glyph's ~0.69 w:h ratio to keep the X square. icon.width/height are
          // int -- a non-integer is a hard QML type error that fails the whole
          // component, so keep these whole numbers (see ui-patterns.md).
          icon.height: 27
          icon.width: 19
          // At rest (no chip) the X must read on the white row, so it's accent;
          // on hover/press the chip fills red and the X goes white.
          icon.color: (deleteBtn.hovered || deleteBtn.down)
                      ? brg.settings.textColorLight
                      : brg.settings.accentColor

          background: Rectangle {
            radius: width / 2
            // No background at rest (Twilight) -- just the accent X. The chip
            // only appears on hover (red) / press (darker red); same mouseover
            // effects as before, only the rest fill is removed.
            color: deleteBtn.down
                   ? Qt.darker(brg.settings.primaryColor, 1.25)
                   : deleteBtn.hovered
                     ? brg.settings.primaryColor
                     : "transparent"
            Behavior on color { ColorAnimation { duration: 90 } }
          }

          onClicked: itemBoxView.model.deleteItem(index, itemChecked === true)
        }
      }

      // Placeholder "+" add row: not draggable, centered. Dropping onto this row
      // appends (its index == count). Visible only while there's room.
      Button {
        anchors.centerIn: parent

        display: AbstractButton.IconOnly
        flat: true

        visible: itemIsPlaceholder && (box.itemsCount < box.itemsMax)

        Material.foreground: brg.settings.primaryColor

        icon.source: "qrc:/assets/icons/fontawesome/plus.svg"

        onClicked: {
          // A hack because insert doesn't work as expected and I have no idea why
          // after a lot of research
          hack_newAndRePositionViewEnd();
        }
      }

      // While being dragged: detach from the row and float in the overlay so the
      // ghost is visible across both panes; fade slightly to read as "lifted".
      states: State {
        when: content.Drag.active
        ParentChange { target: content; parent: itemBoxView.dragLayer }
        AnchorChanges {
          target: content
          anchors.left: undefined
          anchors.verticalCenter: undefined
        }
        PropertyChanges { target: content; opacity: 0.9 }
      }
    }
  }
}
