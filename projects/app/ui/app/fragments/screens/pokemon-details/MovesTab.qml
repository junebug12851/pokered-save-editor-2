// MovesTab.qml -- the "Moves" tab of the Pokemon details editor.
//
// Bound to boxData. The four move slots (boxData.movesAt(0..3)) render as zebra
// rows inside one white bordered panel, matching the General tab's grouped-panel
// design language. Each filled row carries a left grip handle for
// drag-to-reorder; empty slots stay parked at the bottom and aren't draggable
// (game move-list compaction). The drag mirrors the Bag list drag (ItemBoxView):
// the row content reparents to the window overlay so the ghost floats freely, a
// dashed insertion caret marks the drop slot (top edge = before, bottom edge =
// after), and the model mutation (PokemonBox::reorderMove) is deferred a tick so
// no delegate is destroyed mid-drag. See notes/reference/ui-patterns.md.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty
import App.PokemonMove

import "../../general"
import "../../header"
import "../../controls/selection"

Rectangle {
  id: top
  property PokemonBox boxData: null

  color: "transparent"

  // Row height knob + zebra tint, mirroring OverviewTab.
  property int rowH: 44
  property color rowAlt: Qt.rgba(0, 0, 0, 0.04)

  // Floating layer for the drag "ghost": the window overlay, so a row being
  // dragged stays visible and isn't clipped by the ScrollView.
  property Item dragLayer: Overlay.overlay

  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth

    ColumnLayout {
      // -16 reserves the Material scrollbar lane so the row's trailing ⋮ never
      // sits under the overlay scrollbar (see ui-patterns.md -> "Scrollable forms").
      width: scroller.availableWidth - 16
      spacing: 0

      // The single grouped panel holding the four move rows (Bag/General style).
      Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 5
        implicitHeight: rowsCol.implicitHeight
        color: brg.settings.textColorLight
        clip: true

        Column {
          id: rowsCol
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top

          // One DropArea row per move slot. The visible content lives in a child
          // `content` Item that becomes the drag target; while dragging it
          // reparents to the overlay (top.dragLayer). The drag is started ONLY
          // from the left grip handle, and only on filled moves.
          Repeater {
            // Always exactly four move slots (PokemonBox::movesMax()). It's a
            // plain C++ method, not Q_INVOKABLE, so QML can't call it -- use the
            // constant. boxData null-guarded so the panel is empty before binding.
            model: top.boxData ? 4 : 0

            delegate: DropArea {
              id: row
              required property int index

              width: rowsCol.width
              height: top.rowH

              // null-guarded: boxData can read undefined transiently (Repeater
              // re-evaluation / teardown), and every binding below already
              // tolerates a null move, so don't let the lookup itself throw.
              property PokemonMove rowMove: top.boxData ? top.boxData.movesAt(index) : null
              property bool filled: rowMove !== null && rowMove.moveID !== 0
              property int rowIndex: index

              // Which edge the caret sits on / which slot a drop lands in: the
              // pointer's vertical half of the hovered row (top = insert before,
              // bottom = insert after). Updated as the drag moves over the row.
              property bool dropAfter: false
              onPositionChanged: (drag) => { row.dropAfter = drag.y > (row.height / 2); }

              // A move was dropped onto this slot. Only filled source rows reorder
              // (empties have no grip). toIndex = this row (+1 when dropping on its
              // lower half); reorderMove clamps + appends past the end.
              onDropped: (drop) => {
                var s = drop.source;
                if(s === null || s === undefined || s.filledSource !== true)
                  return;

                var fromIndex = s.sourceIndex;
                var toIndex = row.rowIndex + (row.dropAfter ? 1 : 0);

                // Defer the mutation: the dragged content is still parented to the
                // overlay until the release settles; running next tick lets it
                // return to its row first (no delegate destroyed mid-drag).
                Qt.callLater(function() {
                  if(top.boxData)
                    top.boxData.reorderMove(fromIndex, toIndex);
                });
              }

              // Zebra background over the white panel (positional striping). The
              // alt tint is inlined (not top.rowAlt) so the delegate doesn't read
              // through `top` during build/teardown -- that read goes [undefined]
              // transiently and would trip the zero-warning screen-load test.
              Rectangle {
                anchors.fill: parent
                color: (row.index % 2 === 0) ? "transparent" : Qt.rgba(0, 0, 0, 0.04)
              }

              // Insertion caret: a dashed HORIZONTAL bar on the row's top edge
              // (insert before) or bottom edge (insert after). Pure overlay -- it
              // never reflows the rows. Shown only while a filled drag hovers.
              Canvas {
                id: dropHint
                height: 6
                width: parent.width - 20
                anchors.horizontalCenter: parent.horizontalCenter
                y: row.dropAfter ? (row.height - 3) : -3
                z: 300
                visible: row.containsDrag
                onVisibleChanged: requestPaint()
                onYChanged: requestPaint()
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

              // The visible row content. Fills the row at rest; while dragging it
              // reparents to top.dragLayer (the overlay) so the ghost floats free.
              Item {
                id: content
                width: row.width
                height: row.height
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter

                Drag.source: row
                Drag.hotSpot.x: 24
                Drag.hotSpot.y: height / 2

                // Info the drop target reads off the dragged row.
                property bool filledSource: row.filled
                property int sourceIndex: row.rowIndex

                // Drive the Drag manually: an internal MouseArea drag never
                // auto-commits, so we call Drag.drop() ourselves on release (then
                // clear Drag.active to revert the lift). Mirrors ItemBoxView.
                property bool maActive: dragHandler.drag.active
                onMaActiveChanged: {
                  if(maActive) {
                    content.Drag.active = true;
                  }
                  else if(content.Drag.active) {
                    content.Drag.drop();
                    content.Drag.active = false;
                  }
                }

                // While being dragged: a clean "card" so the floating row reads as
                // lifted.
                Rectangle {
                  anchors.fill: parent
                  visible: content.Drag.active
                  color: brg.settings.textColorLight
                  radius: 4
                  border.color: brg.settings.accentColor
                  border.width: 1
                  z: -1
                }

                RowLayout {
                  anchors.fill: parent
                  anchors.leftMargin: 4
                  spacing: 0

                  // Grip handle -- the ONLY drag initiator, and only on filled
                  // moves (the rest of the row stays clickable/typable; empty
                  // slots aren't draggable). The column is always reserved (width
                  // 24) so every row's combo lines up.
                  Item {
                    id: grip
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: row.height

                    Image {
                      anchors.centerIn: parent
                      width: 18
                      height: 18
                      sourceSize.width: width
                      sourceSize.height: height
                      source: "qrc:/assets/icons/fontawesome/grip-lines.svg"
                      fillMode: Image.PreserveAspectFit
                      visible: row.filled
                      opacity: dragHandler.containsMouse ? 0.8 : 0.4
                      Behavior on opacity { NumberAnimation { duration: 90 } }
                    }

                    MouseArea {
                      id: dragHandler
                      anchors.fill: parent
                      enabled: row.filled
                      hoverEnabled: true
                      cursorShape: Qt.OpenHandCursor
                      preventStealing: true
                      drag.target: row.filled ? content : null
                      drag.threshold: 8
                    }
                  }

                  PokemonMoveSel {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    monMove: row.rowMove
                    // Coerce a transient [undefined] read of top.boxData to null
                    // (assignable to PokemonBox*); rowH from row.height avoids the
                    // through-`top` read entirely (zero-warning load test).
                    boxData: top.boxData ? top.boxData : null
                    rowH: row.height
                  }
                }

                // While dragging: detach to the overlay and fade slightly.
                states: State {
                  when: content.Drag.active
                  ParentChange { target: content; parent: top.dragLayer }
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
        }
      }
    }
  }
}
