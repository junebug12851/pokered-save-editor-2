// PokemonBoxView.qml -- the grid of Pokemon slots inside a PokemonPane.
//
// A GridView of mon cells over a PokemonStorageModel (theModel). Each filled cell
// shows the species/shiny icon, a level badge, a hover checkbox, and an
// always-visible name label below the icon (mon nickname, or species name as
// fallback) in dark text on no background; clicking anywhere on the cell opens
// PokemonDetails.qml for that mon. Empty cells show a "+" add button.
// openMonEditor pushes the details page manually (to pass the mon as a parameter)
// and wires router/page close listeners so the model resets when the editor
// closes. The (itemDex+1) padding mirrors Pokedex.qml's correct 0->1 dex
// conversion; the inline notes on the custom text label and SVG tinting are real
// workarounds -- leave them. See notes/reference/ui-patterns.md.
//
// Drag & drop: each cell is a DropArea; its content is a drag target that
// reparents to the window overlay while dragging (so the ghost floats across
// both panes). Dropping reorders within the pane (dragReorder) or transfers in
// from the other pane (dragTransfer); the drop slot is the targeted cell (insert
// before it; the trailing "+" slot appends). A press shorter than drag.threshold
// stays a click and opens the editor. If the grabbed mon is checked, the whole
// checked set moves (group drag). A dashed Canvas marks the hovered drop slot.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonStorageModel
import App.PokemonBoxSelectModel

import "../../general"
import "../../controls/selection"

GridView {
  id: view
  property int cellSize: 100
  property PokemonStorageModel theModel: null

  // Floating layer for the drag "ghost": the window overlay, so a mon being
  // dragged stays visible across BOTH panes and isn't clipped by this GridView.
  property Item dragLayer: Overlay.overlay

  cellWidth: cellSize
  cellHeight: cellSize
  clip: true

  function hack_newAndRePositionViewEnd() {
    theModel.getCurBox().pokemonNew();
    positionViewAtEnd();
  }

  // We manually open outside of the router the pokemon details page beacuse
  // we want to pass parameters to it
  function openMonEditor(isParty, monData) {

    // Will fix this in a minute
    if(isParty)
      appBody.push("qrc:/ui/app/screens/non-modal/PokemonDetails.qml", {
                     boxData: monData,
                     partyData: monData
                   });
    else
      appBody.push("qrc:/ui/app/screens/non-modal/PokemonDetails.qml", {
                     boxData: monData
                   });

    // We then tell the router of what we've done
    brg.router.manualStackPush("pokemonDetails");

    // And then we open incomming signals so that we can receive input
    // This is tricky, we first enable listening for a page close event from the
    // router and then we enable listening from the details page. When the
    // details page closes we want both to shut off. This is how it's done
    pokemonDetailsListenerShutOff.target = brg.router;
    pokemonDetailsListener.target = appRoot.currentItem;
  }

  // Incomming signals from full-keyboard
  Connections {
    id: pokemonDetailsListener

    // Initially set to no incomming signals
    target: null
    ignoreUnknownSignals: true
  }

  // Here we shut-off connections
  Connections {
    id: pokemonDetailsListenerShutOff

    target: null
    ignoreUnknownSignals: true

    function onCloseNonModal() {
      theModel.onReset();
      pokemonDetailsListener.target = null;
      pokemonDetailsListenerShutOff.target = null;
    }
  }

  // Each cell is a DropArea so a mon can be dropped onto it (drop-to-commit, no
  // live reshuffle). The visible content lives in a child `content` Item that
  // becomes the drag target; while dragging it reparents to the overlay so the
  // ghost floats across both panes. A plain click (below drag.threshold) still
  // opens the editor. Group drags (the grabbed mon is checked) carry the whole
  // checked set, handled by the model's dragReorder/dragTransfer.
  delegate: DropArea {
    id: cell
    width: view.cellSize
    height: view.cellSize

    // --- Info the drop target reads off the dragged item (drop.source) ---
    property var ownerModel: view.theModel
    property int cellIndex: index
    property bool grabbedChecked: (itemChecked === true)
    property bool isPlaceholder: (itemIsPlaceholder === true)

    function getMonUrl() {

      if(itemDex === -1 || itemDex === undefined || itemDex === null)
        return "qrc:/assets/icons/fontawesome/question.svg";

      var num = (itemDex+1).toString().padStart(3, "0");

      var name = itemName.toLowerCase();
      if(name === "nidoran<f>")
        name = "nidoran-f";
      else if(name === "nidoran<m>")
        name = "nidoran-m";
      else if(name === "mr.mime")
        name = "mrmime";

      var shiny = (itemIsShiny)
          ? "-shiny"
          : ""

      return "qrc:/assets/icons/mon-icons/" + num + "-" + name + shiny + ".svg";
    }

    function getMonNickname() {
      // Show the nickname; for an un-nicknamed mon (empty nickname) fall back to
      // the species name, matching the in-game display. (Most mons in a save have
      // no custom nickname, so without this the label is just blank.)
      var nick = (itemNickname === undefined || itemNickname === null) ? "" : itemNickname;
      if(nick === "")
        nick = (itemName === undefined || itemName === null) ? "" : itemName;

      if(nick.length > 10)
        return nick.substring(0, 7) + "...";
      else
        return nick;
    }

    // A mon was dropped onto this slot: reorder within the pane, or transfer in
    // from the other pane. toIndex == this slot (insert before it); the trailing
    // placeholder slot's index == count, so dropping there appends.
    onDropped: (drop) => {
      var s = drop.source;
      if(s === null || s === undefined || s.isPlaceholder === true)
        return;

      var srcModel = s.ownerModel;
      var srcIndex = s.cellIndex;
      var grp = (s.grabbedChecked === true);
      var toIndex = cell.cellIndex;
      var samePane = (srcModel === view.theModel);

      // Defer the model mutation: the drop resets the model (rebuilding these
      // delegates), but the dragged content is still parented to the overlay
      // until the release settles. Running it next tick lets the ghost return to
      // its cell first, so no delegate is destroyed mid-drag (no orphaned ghost).
      Qt.callLater(function() {
        if(samePane)
          srcModel.dragReorder(srcIndex, toIndex, grp);
        else
          srcModel.dragTransfer(srcIndex, toIndex, grp);
      });
    }

    // Insertion caret: a dashed vertical bar straddling this cell's LEFT edge
    // (the gap *before* it), shown while a drag hovers this slot. It's an overlay
    // -- it never resizes or shuffles the icons. The drop inserts before this
    // cell; hovering the trailing "+" slot marks "after the last entry" (its left
    // edge is the gap between the last mon and the New button).
    Canvas {
      id: dropHint
      width: 6
      height: parent.height - 14
      anchors.left: parent.left
      anchors.leftMargin: -3            // center the bar on the cell boundary
      anchors.verticalCenter: parent.verticalCenter
      z: 300
      visible: cell.containsDrag
      onVisibleChanged: requestPaint()
      onHeightChanged: requestPaint()
      onPaint: {
        var ctx = getContext("2d");
        ctx.reset();
        ctx.clearRect(0, 0, width, height);
        ctx.strokeStyle = brg.settings.accentColor;
        ctx.lineWidth = 3;
        ctx.lineCap = "round";
        ctx.setLineDash([5, 4]);
        ctx.beginPath();
        ctx.moveTo(width / 2, 2);
        ctx.lineTo(width / 2, height - 2);
        ctx.stroke();
      }
    }

    // The visible cell content. Centered in the cell at rest; while dragging it
    // reparents to view.dragLayer (the overlay) and its center anchors clear so
    // the drag handler can move it freely.
    Item {
      id: content
      width: view.cellSize
      height: view.cellSize
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.verticalCenter: parent.verticalCenter

      Drag.source: cell
      Drag.hotSpot.x: width / 2
      Drag.hotSpot.y: height / 2

      // Drive the Drag manually. An internal MouseArea drag never auto-commits,
      // so DropArea.onDropped won't fire on its own -- we have to call Drag.drop()
      // ourselves on release. We hold Drag.active true from when the MouseArea
      // drag begins until we've dropped, so the drop lands on the DropArea under
      // the cursor; setting it false then reverts the lift state (reparent back).
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

      // Whole-cell hover. Driving the checkbox/delete visibility off a
      // HoverHandler (not dragHandler.containsMouse) keeps them shown when the
      // pointer moves onto a child control -- a hovered Button/CheckBox would
      // otherwise steal hover from the MouseArea and hide the very button you're
      // reaching for.
      HoverHandler { id: cellHover }

      CheckBox {
        id: selectBox

        visible: !itemIsPlaceholder && (cellHover.hovered || checked)

        anchors.top: parent.top
        anchors.left: parent.left

        // Bind to the model role (persists across box switches / model resets,
        // since delegates are recreated with a fresh binding); write back only on
        // a user toggle. The old one-shot Component.onCompleted didn't restore on
        // reuse, so checks "disappeared".
        checked: (itemChecked === true)
        onToggled: itemChecked = checked

        z: 100

        Material.background: brg.settings.accentColor
        Material.foreground: brg.settings.textColorLight
      }

      // Always-visible name label below the icon: nickname (or species name as
      // fallback), dark text on no background. The cell-wide MouseArea handles the
      // click that opens the editor, so this is purely a label.
      Text {
        id: nameLabel
        visible: !itemIsPlaceholder

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        anchors.left: parent.left
        anchors.right: parent.right

        text: cell.getMonNickname()
        color: brg.settings.textColorDark
        font.capitalization: Font.MixedCase
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
      }

      MouseArea {
        id: dragHandler

        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true            // keep the GridView flick from stealing the drag

        // Only real mons are draggable; the "+" slot keeps its add button.
        drag.target: cell.isPlaceholder ? null : content
        drag.threshold: 8                // a press that moves less than this stays a click

        // A plain click (no drag past the threshold) opens the editor, exactly as
        // before; a completed drag suppresses the click automatically. Placeholder
        // "+" slots keep their own add handler (the RoundButton below).
        onClicked: {
          if(itemIsPlaceholder)
            return;
          if(itemIsParty)
            openMonEditor(itemIsParty, view.theModel.getPartyMon(index));
          else
            openMonEditor(itemIsParty, view.theModel.getBoxMon(index));
        }
      }

      Image {
        visible: !itemIsPlaceholder

        // Fills the cell above the name label, so the icon and its name read as one
        // stacked unit.
        anchors.top: parent.top
        anchors.bottom: nameLabel.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8

        sourceSize.height: height
        sourceSize.width: width

        source: cell.getMonUrl()
        fillMode: Image.PreserveAspectFit
      }

      Rectangle {
        visible: !itemIsPlaceholder

        anchors.top: parent.top
        anchors.topMargin: 15
        anchors.right: parent.right

        radius: 20
        color: brg.settings.accentColor
        width: 12 * 3
        height: 15

        Text {
          anchors.centerIn: parent

          color: brg.settings.textColorLight
          font.pixelSize: 12
          text: "L" + itemLevel
        }
      }

      RoundButton {
        anchors.centerIn: parent
        visible: itemIsPlaceholder
        width: parent.width * 0.60
        height: parent.height * 0.60
        radius: 100
        display: AbstractButton.IconOnly
        padding: 0
        topInset: 0
        rightInset: 0
        bottomInset: 0
        leftInset: 0
        flat: true

        icon.source: "qrc:/assets/icons/fontawesome/plus.svg"

        Material.background: brg.settings.primaryColor
        Material.foreground: brg.settings.textColorLight

        onClicked: hack_newAndRePositionViewEnd()
      }

      // Delete button: a real round chip at the bottom-right, shown on hover OR
      // when checked (mirrors the top-left checkbox via the same HoverHandler).
      // At rest it's a faint outlined circle with a red "times" glyph (Twilight's
      // release icon, not a trash can); on hover the circle fills red and the
      // glyph goes white; on press it darkens -- so it clearly reads and feels
      // like a button. On a checked mon it deletes the whole checked set (the
      // replacement for the old footer "release" bulk button); otherwise just
      // this mon. z above the drag handler so the click lands here.
      Button {
        id: deleteBtn
        visible: !itemIsPlaceholder && (cellHover.hovered || itemChecked === true)
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 5
        anchors.bottomMargin: 5
        z: 100

        width: 28
        height: 28
        padding: 0
        topInset: 0
        bottomInset: 0
        leftInset: 0
        rightInset: 0
        display: AbstractButton.IconOnly

        icon.source: "qrc:/assets/icons/fontawesome/times.svg"
        // The icon is STRETCHED to fill icon.width x icon.height (a square box made
        // the X look wide), so use the glyph's ~0.6875 w:h ratio to keep the X
        // square, and make the box large (in a slightly bigger 28px chip) so the
        // visible X is actually big. NB: icon.width/height are int -- a non-integer
        // (e.g. 13.75) is a hard QML type error that fails the whole component, so
        // keep these whole numbers.
        icon.height: 27
        icon.width: 19
        // Always white so it reads on both the accent rest chip and the red
        // hover/press fill.
        icon.color: brg.settings.textColorLight

        background: Rectangle {
          radius: width / 2
          // Opaque accent chip at rest (the in-screen "menu bar" colour) -- only
          // shown on hover/checked, so a full chip reads cleanly; fills red on
          // hover, darker red on press.
          color: deleteBtn.down
                 ? Qt.darker(brg.settings.primaryColor, 1.25)
                 : deleteBtn.hovered
                   ? brg.settings.primaryColor
                   : brg.settings.accentColor
          Behavior on color { ColorAnimation { duration: 90 } }
        }

        onClicked: view.theModel.deleteMon(index, itemChecked === true)
      }

      // While being dragged: detach from the cell and float in the overlay so the
      // ghost is visible across both panes; fade slightly to read as "lifted".
      states: State {
        when: content.Drag.active
        ParentChange { target: content; parent: view.dragLayer }
        AnchorChanges {
          target: content
          anchors.horizontalCenter: undefined
          anchors.verticalCenter: undefined
        }
        PropertyChanges { target: content; opacity: 0.85 }
      }
    }
  }

  ScrollBar.vertical: ScrollBar {}
}
