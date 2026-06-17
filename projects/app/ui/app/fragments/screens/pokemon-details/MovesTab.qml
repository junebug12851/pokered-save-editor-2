// MovesTab.qml -- the "Moves" tab of the Pokemon details editor.
//
// Bound to boxData. The four move slots (boxData.movesAt(0..3)) render as zebra
// rows inside one white bordered panel, matching the General tab's grouped-panel
// design language. Each filled row carries a left grip handle for
// drag-to-reorder; empty slots stay parked at the bottom and aren't draggable
// (game move-list compaction). The drag mirrors the Bag list drag (ItemBoxView):
// the row content reparents to the window overlay so the ghost floats freely, a
// dashed insertion caret marks the drop slot (top edge = before, bottom = after),
// and the model mutation (PokemonBox::reorderMove) is deferred a tick so no
// delegate is destroyed mid-drag. See notes/reference/ui-patterns.md.
//
// NOTE the root id is `movesTab`, NOT `top`: PokemonMoveSel (instantiated in the
// Repeater delegate below) also uses `id: top`, and inside the delegate that inner
// `top` shadows the root -- so `top.boxData` there resolved to PokemonMoveSel's own
// (null) boxData, leaving every row blank. A unique root id avoids the collision.
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
  id: movesTab
  property PokemonBox boxData: null

  color: "transparent"

  // Row height knob + zebra tint, mirroring OverviewTab.
  property int rowH: 44
  property color rowAlt: Qt.rgba(0, 0, 0, 0.04)

  // Floating layer for the drag "ghost": the window overlay, so a row being
  // dragged stays visible and isn't clipped by the ScrollView.
  property Item dragLayer: Overlay.overlay

  // Tab-level view toggle: false = each row edits current/max PP; true = current/
  // max PP-Ups. One state for the whole tab, so it stays consistent as moves are
  // dragged around (the values follow their move; the view doesn't change).
  property bool showPpUps: false

  // Selectable text segment of a connected toggle group (matches the DV/EV tab's
  // SegSel). `active` is bound to data, not a checkable state.
  component SegSel: Button {
    id: ssel
    property bool active: false
    property bool first: false
    property bool last: false
    property string tip: ""
    flat: true
    display: AbstractButton.TextOnly
    topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
    leftPadding: 16; rightPadding: 16; topPadding: 0; bottomPadding: 0
    Layout.fillHeight: true
    Layout.minimumHeight: 0
    font.pixelSize: 13
    contentItem: Text {
      text: ssel.text
      font: ssel.font
      color: ssel.active ? brg.settings.textColorLight : brg.settings.textColorMid
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
      color: ssel.active ? brg.settings.accentColor
             : ssel.down ? Qt.rgba(0, 0, 0, 0.16)
             : ssel.hovered ? Qt.rgba(0, 0, 0, 0.08)
             : "transparent"
      topLeftRadius: ssel.first ? 4 : 0
      bottomLeftRadius: ssel.first ? 4 : 0
      topRightRadius: ssel.last ? 4 : 0
      bottomRightRadius: ssel.last ? 4 : 0
      Rectangle {
        visible: !ssel.first && !ssel.active
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: 1
        color: Qt.rgba(0, 0, 0, 0.15)
      }
    }
    MainToolTip { text: ssel.tip }
  }

  // Icon segment of a connected action group (matches the DV/EV tab's SegBtn).
  component SegBtn: Button {
    id: seg
    property bool first: false
    property bool last: false
    property string tip: ""
    flat: true
    display: AbstractButton.IconOnly
    topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
    padding: 7
    icon.color: brg.settings.textColorDark
    Layout.fillHeight: true
    Layout.minimumHeight: 0
    background: Rectangle {
      color: seg.down ? Qt.rgba(0, 0, 0, 0.16)
             : seg.hovered ? Qt.rgba(0, 0, 0, 0.08)
             : "transparent"
      topLeftRadius: seg.first ? 4 : 0
      bottomLeftRadius: seg.first ? 4 : 0
      topRightRadius: seg.last ? 4 : 0
      bottomRightRadius: seg.last ? 4 : 0
      Rectangle {
        visible: !seg.first
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: 1
        color: Qt.rgba(0, 0, 0, 0.15)
      }
    }
    MainToolTip { text: seg.tip }
  }

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

        // A ColumnLayout (not a plain Column): the rows use Layout.fillWidth to
        // take the panel's width. A plain Column auto-sizes its width FROM its
        // children, so a child bound to `rowsCol.width` is circular and collapses
        // the rows to zero width (the "empty Moves tab" bug). Mirrors OverviewTab.
        ColumnLayout {
          id: rowsCol
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          spacing: 0

          // ---- Top bar: PP / PP-Ups view toggle (left) + bulk actions (right):
          //      clear-all-but-first (broom), randomize all (dice), make all valid
          //      (check-double). ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "transparent"

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 10
              anchors.rightMargin: 10
              spacing: 10

              // Segmented PP | PP Ups.
              Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 32
                implicitWidth: viewGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: viewGrp
                  anchors.fill: parent
                  spacing: 0
                  SegSel {
                    first: true
                    text: qsTr("PP")
                    active: !movesTab.showPpUps
                    onClicked: movesTab.showPpUps = false
                    tip: qsTr("Edit each move's current / max PP.")
                  }
                  SegSel {
                    last: true
                    text: qsTr("PP Ups")
                    active: movesTab.showPpUps
                    onClicked: movesTab.showPpUps = true
                    tip: qsTr("Edit each move's PP-Ups (0–3) instead of PP.")
                  }
                }
              }

              Item { Layout.fillWidth: true }

              // Clear-all-but-first | Randomize all | Make all valid.
              Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 32
                implicitWidth: bulkGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: bulkGrp
                  anchors.fill: parent
                  spacing: 0
                  SegBtn {
                    first: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/broom.svg"
                    // Enabled only when there's a 2nd move to clear. movesCount() is
                    // a plain C++ method (not QML-callable), so test slot 1 directly
                    // -- movesAt(1).moveID has a NOTIFY, so this stays reactive.
                    enabled: movesTab.boxData ? movesTab.boxData.movesAt(1).moveID > 0 : false
                    onClicked: if(movesTab.boxData) movesTab.boxData.clearMovesButFirst();
                    tip: qsTr("Clear every move except the first.")
                  }
                  SegBtn {
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
                    onClicked: if(movesTab.boxData) movesTab.boxData.randomizeMoves();
                    tip: qsTr("Replace all moves with a random valid set.")
                  }
                  SegBtn {
                    last: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/check-double.svg"
                    onClicked: if(movesTab.boxData) movesTab.boxData.correctMoves();
                    tip: qsTr("Make every move valid for the Pokémon.")
                  }
                }
              }
            }
          }

          // One DropArea row per move slot. The visible content lives in a child
          // `content` Item that becomes the drag target; while dragging it
          // reparents to the overlay (movesTab.dragLayer). The drag is started ONLY
          // from the left grip handle, and only on filled moves.
          Repeater {
            // Always exactly four move slots (PokemonBox::movesMax()). It's a
            // plain C++ method, not Q_INVOKABLE, so QML can't call it -- use the
            // constant. boxData null-guarded so the panel is empty before binding.
            model: movesTab.boxData ? 4 : 0

            delegate: DropArea {
              id: row
              required property int index

              // Row height is a delegate-local constant (NOT movesTab.rowH read
              // through the root id from inside the delegate -- that goes
              // [undefined] transiently and collapses the row). Keep in sync.
              readonly property int rowHeight: 44

              Layout.fillWidth: true
              Layout.preferredHeight: rowHeight

              // null-guarded: boxData can read undefined transiently (Repeater
              // re-evaluation / teardown), and every binding below already
              // tolerates a null move, so don't let the lookup itself throw.
              property PokemonMove rowMove: movesTab.boxData ? movesTab.boxData.movesAt(index) : null
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
                  if(movesTab.boxData)
                    movesTab.boxData.reorderMove(fromIndex, toIndex);
                });
              }

              // Zebra background over the white panel (positional striping). The
              // alt tint is inlined (not movesTab.rowAlt) so the delegate doesn't
              // read through the root id during build/teardown -- that read goes
              // [undefined] transiently and would trip the zero-warning load test.
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
              // reparents to movesTab.dragLayer (the overlay) so the ghost floats free.
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
                    boxData: movesTab.boxData
                    moveIndex: row.index
                    showPpUps: movesTab.showPpUps
                    rowH: row.height
                  }
                }

                // While dragging: detach to the overlay and fade slightly.
                states: State {
                  when: content.Drag.active
                  ParentChange { target: content; parent: movesTab.dragLayer }
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
