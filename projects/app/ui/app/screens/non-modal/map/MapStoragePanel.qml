/*
  * Copyright 2026 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/**
 * ▣ **MAP STORAGE** -- persistent save values that each belong to one specific map.
 *
 * A per-map VIEW over global save state. Every page is one place, and shows (top to bottom):
 *
 *   1. **The map's SCRIPT** -- its per-map script-progress value (`w<Map>CurScript`, the 97-byte
 *      block at save 0x289C, `world.scripts`). A dropdown of the map's named steps with a
 *      progression description each; "Something else…" opens the raw path (full byte range, with
 *      a WARNING beyond the map's own table -- steps dispatch through an unbounded jp-hl pointer
 *      table, so an out-of-range step is a real crash risk; shown, never refused).
 *   2. **The legacy minigame bytes** where the map has them (Vermilion trash cans, Cinnabar quiz,
 *      Safari counters -- `world.local`; notes/reference/gym-safari-state.md).
 *   3. **The map's EVENT FLAGS** -- what has HAPPENED here (`wEventFlags`, ONE contiguous 320-byte
 *      field at save 0x29F3; bit `i` = byte 0x29F3 + i/8, bit i%8; `world.events`). Grouped, every
 *      group open (it is a page you scroll), each row a switch + description + a caution where one
 *      applies, with a **group toggle** per group. Every flag is filed on its OWN map; one that
 *      spans several is **SHARED** and appears on each of their pages in a labelled shared group
 *      naming the others -- shared groups are a panel-level idea and can be of several TYPES (event
 *      flags here; storage bytes elsewhere). **Placeholder Flags** (spare bits the game never reads)
 *      come last. Nothing about conflicts is shown -- that system is shelved.
 *      See notes/reference/event-flags.md.
 *   4. **The map's FILTER FLAGS** (Twilight's term; pret/the game call them "missables") -- its
 *      hide/show object bits (`wToggleableObjectFlags`, 32 bytes at save 0x2852, `world.missables`;
 *      bit SET = HIDDEN). Each a Shown switch with a description,
 *      pret's known-issue oddities flagged amber, and the linked event flags surfaced live -- read-only
 *      context, so you can see at a glance when a flag says "done" beside an object that says otherwise.
 *      (That pairing was once meant to feed a conflicting-flags system; that system is SHELVED --
 *      2026-07-16, see notes/decisions/rejected.md -- so this is informational only and nothing here
 *      judges a combination as a conflict.)
 *
 * Everything full-range, hack values shown and never silently rewritten.
 * Reference: notes/reference/map-scripts-missables.md.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: panel
  objectName: "mapStorage"

  /// The panel's one title-bar "?" (MapDock renders it).
  readonly property string panelInfo: qsTr(
    "Everything the save stores about one specific place: the map's script progress (how far its "
    + "story events have advanced), its minigame values (trash cans, quiz, Safari counters), and "
    + "its Filter Flags — which people, items and Pokémon are on the map at all.\n\n"
    + "Pick a map; the one you're standing on is pre-selected when it has storage. Script steps "
    + "read like a story — each option says what's happening at that stage.\n\n"
    + "Custom values are always available and never refused; values the game can't handle are "
    + "flagged in words instead.")

  /// Harness-visible: the panel resolved its model.
  readonly property bool diagReady: panel.local !== null && panel.local !== undefined

  // BINDINGS, not an imperative refresh -- rebuilt by a Loader, and the models must re-resolve when
  // a different map/save loads. `revision` is the dependency a plain property access would miss;
  // `editTick` re-reads values after our own writes (invokable reads aren't notified).
  property int revision: 0
  property int editTick: 0
  Connections {
    target: brg.map
    function onChanged() { panel.revision++; panel.syncToCurrentMap(); }
  }

  readonly property var local: {
    panel.revision;   // a dependency, deliberately
    return brg.file.data.dataExpanded.world.local;
  }
  readonly property var wScripts: {
    panel.revision;
    return brg.file.data.dataExpanded.world.scripts;
  }
  readonly property var wMissables: {
    panel.revision;
    return brg.file.data.dataExpanded.world.missables;
  }
  /// The "already collected" bits for hidden items + coins (0x299C / 0x29AA). Two separate arrays
  /// behind one node: hItemsAt/Set and hCoinsAt/Set. @see WorldHidden
  readonly property var wHidden: {
    panel.revision;
    return brg.file.data.dataExpanded.world.hidden;
  }
  readonly property var wEvents: {
    panel.revision;
    return brg.file.data.dataExpanded.world.events;
  }
  /// The in-game trade "done" bits (0x29E3). tradesAt/tradesSet(ind). @see WorldTrades
  readonly property var wTrades: {
    panel.revision;
    return brg.file.data.dataExpanded.world.trades;
  }
  /// The town "visited" (Fly-unlock) bits (0x29B7). townsAt/townsSet(ind); bit == map id. @see WorldTowns
  readonly property var wTowns: {
    panel.revision;
    return brg.file.data.dataExpanded.world.towns;
  }
  /// The "completed" one-shots (rods/Lapras/starter/nurse/guards/elite4). Keyed Q_PROPERTY bools:
  /// wCompleted[key] reads/writes. @see WorldCompleted
  readonly property var wCompleted: {
    panel.revision;
    return brg.file.data.dataExpanded.world.completed;
  }
  /// Odds & ends incl. the two fossil bytes (0x29BB/0x29BC). Keyed int Q_PROPERTYs. @see WorldOther
  readonly property var wOther: {
    panel.revision;
    return brg.file.data.dataExpanded.world.other;
  }

  // ── The pages: every map with storage (script progress, minigame bytes, or missables) ─────────
  // Built by MapModel::storagePages(): one page per script-progress entry (97), plus missable-only
  // maps, with the legacy trio merged in (Safari COMBINED — Twilight, 2026-07-15).
  readonly property var storageMaps: { panel.revision; return brg.map.storagePages(); }

  property int page: 0   // index into storageMaps -- which map's storage is shown

  // Switching pages starts at the TOP -- a page opened at wherever the last one was scrolled reads
  // as broken ("opens scrolled down way too low"). A reveal mid-flight owns the scroll instead.
  onPageChanged: if (!panel.revealSettling) scroller.contentItem.contentY = 0

  function pageForMap(mapInd) {
    for (let i = 0; i < storageMaps.length; i++)
      if (storageMaps[i].ids.indexOf(mapInd) !== -1)
        return i;
    return -1;
  }
  /// Pre-select the combo to the map you're standing on, when it has storage.
  function syncToCurrentMap() {
    const i = pageForMap(brg.map.mapInd);
    if (i !== -1)
      panel.page = i;
  }
  Component.onCompleted: syncToCurrentMap()

  // ── Opened AT something: the canvas clicked a flag box ────────────────────────────────────────
  //
  // "have a box around it on the map that you can click which opens the persistent storage panel at
  // the thing clicked" (Fairy Fox). Landing on the page is not enough -- a busy map's page is long
  // (Silph Co 7F runs to 227 rows), so arriving at the top and leaving her to hunt for the row would
  // miss the point of the gesture entirely. So: switch to the map, then scroll to the row and light
  // it up.

  /// The missable bit to highlight, or -1. @see reveal
  property int highlightMissable: -1
  /// Which section the highlight is in ("missable" / "event" / "script" / "hidden"), or "".
  property string highlightSection: ""
  /// The row index to highlight within @ref highlightSection, or -1.
  property int highlightInd: -1

  /// Open this panel ON row @p ind of @p section. Called by Map.qml when a spot or a tab is reached.
  ///
  /// One entry point for every storage kind, keyed by the section the spot says it lives in --
  /// so the next kind of storage needs a spot type in the model, not another reveal function.
  function reveal(section, ind) {
    // The box is on the map you are looking at, and the combo may have been moved elsewhere.
    syncToCurrentMap();
    panel.highlightSection = section;
    panel.highlightInd = ind;
    // Kept for the missable rows, which already bind to it.
    panel.highlightMissable = (section === "missable") ? ind : -1;
    highlightFade.restart();
    // ⚠️ NOT Qt.callLater: on the FIRST open the panel is being loaded and built in this same tick,
    // and a callLater still runs before the ColumnLayout has given its children their heights --
    // so mapToItem() answers with a stale y and the scroll lands at the top. The settle window
    // below re-applies the scroll while the layout is still growing, then stands down.
    panel.revealSettling = true;
    revealScroll.restart();
    revealSettle.restart();
  }

  /// @deprecated Use reveal("missable", ind). Kept so an older caller cannot silently do nothing.
  function revealMissable(ind) { panel.reveal("missable", ind); }

  /// ⭐ THE SETTLE WINDOW. A freshly opened page builds in stages (Loaders, Repeaters, images), so
  /// any single one-shot scroll races the layout -- which is exactly the *"opens with the content
  /// scrolled weird, significant white space, then it appears properly after some time"* and the
  /// *"scrolls up too high, sits for a while, then scrolls back down"* leadership hit (2026-07-18).
  /// The fix is not a longer wait (that IS the visible sitting): apply the scroll immediately,
  /// then RE-apply it whenever the content's height moves during a short window, and stop. The
  /// panel therefore tracks its target while the page grows underneath it, instead of jumping to
  /// a stale position and correcting later.
  property bool revealSettling: false

  Timer {
    id: revealScroll
    interval: 16               // one frame -- just enough for the synchronous build to land
    onTriggered: panel.scrollToHighlight()
  }
  Timer {
    id: revealSettle
    interval: 450              // the settle window's end
    onTriggered: panel.revealSettling = false
  }
  Connections {
    target: scroller.contentItem
    enabled: panel.revealSettling
    function onContentHeightChanged() { panel.scrollToHighlight(); }
  }

  /// The section item a reveal should scroll to. Unknown section -> null, and the scroll is skipped
  /// rather than guessing at one.
  function sectionItem(section) {
    switch (section) {
      case "missable": return missableSection;
      case "event":    return eventSection;
      case "script":   return scriptSection;
      case "hidden":   return hiddenSection;
    }
    return null;
  }

  function scrollToHighlight() {
    const target = panel.sectionItem(panel.highlightSection);
    if (target === null || !target.visible)
      return;

    // ⚠️ THE FLICKABLE TRAP. `scroller.contentItem` is the Flickable, and mapToItem() to a Flickable
    // answers in its VIEWPORT's coordinates -- which already have contentY taken off. So the number
    // that comes back is where the section is on SCREEN right now, not where it sits in the layout.
    // Adding the current contentY back converts viewport -> content space.
    //
    // ⚠️ And it maps THE SECTION ASKED FOR. This line read `missableSection.mapToItem(...)` --
    // hardcoded -- so revealing a script, an event flag or a hidden pickup scrolled to wherever the
    // MISSABLES happened to sit: sometimes way past the target, into white space the layout hadn't
    // filled yet. That one word was most of the "glitched" open. (Leadership, 2026-07-18.)
    const rel = target.mapToItem(scroller.contentItem, 0, 0).y;
    let want = scroller.contentItem.contentY + rel - 8;
    // Clamp to the real content ONLY when the layout has one -- a clamp against an uncomputed
    // contentHeight pins the panel to the top, which is the other half of the old bug. The settle
    // window re-applies as the height grows, so an early over-scroll self-corrects immediately.
    const ch = scroller.contentItem.contentHeight;
    if (ch > scroller.height)
      want = Math.min(want, ch - scroller.height);
    scroller.contentItem.contentY = Math.max(0, want);
  }

  /// The highlight is a pointer, not a state -- it says "here", then gets out of the way.
  Timer {
    id: highlightFade
    interval: 2600
    onTriggered: panel.highlightMissable = -1
  }

  readonly property var curPage: storageMaps[panel.page]

  /// Is the shown map the one you're actually on? (Drives the "you're here" / "not here" note.)
  readonly property bool onShownMap: {
    panel.revision;
    return curPage !== undefined && curPage.ids.indexOf(brg.map.mapInd) !== -1;
  }

  // ── field kit ─────────────────────────────────────────────────────────────────────────────────

  // A numeric value: title, a compact right-aligned number field, and a plain-English line. Full
  // storable range is accepted (hack values shown, never refused -- project rule); the blurb names
  // the range a real game uses.
  component NumRow: ColumnLayout {
    id: nrow
    property string title: ""
    property string blurb: ""
    property int value: 0
    property int maxStorable: 255      // a byte; the Safari step counter is a word (65535)
    signal setValue(int v)

    Layout.fillWidth: true
    spacing: 1

    RowLayout {
      Layout.fillWidth: true
      spacing: 6

      Label {
        text: nrow.title
        font.pixelSize: 12
        color: brg.settings.textColorDark
        Layout.fillWidth: true
        wrapMode: Text.Wrap
      }

      TextField {
        Layout.preferredWidth: 64
        font.pixelSize: 12
        horizontalAlignment: TextInput.AlignRight
        text: nrow.value
        inputMethodHints: Qt.ImhDigitsOnly
        validator: IntValidator { bottom: 0; top: nrow.maxStorable }
        onEditingFinished: {
          const v = parseInt(text);
          if (!isNaN(v))
            nrow.setValue(Math.max(0, Math.min(nrow.maxStorable, v)));
        }
      }
    }

    Label {
      text: nrow.blurb
      Layout.fillWidth: true
      wrapMode: Text.Wrap
      font.pixelSize: 10
      opacity: 0.55
    }
  }

  // A boolean value the game REWRITES on its own -- shown, but flagged temporary (the ⚠ is a label,
  // never a reason to hide; the Character panel set the precedent).
  component TempFlagRow: ColumnLayout {
    id: trow
    property string title: ""
    property string blurb: ""
    property string tempWhy: ""
    property bool value: false
    signal toggle()

    Layout.fillWidth: true
    spacing: 1

    RowLayout {
      Layout.fillWidth: true
      spacing: 6

      MapWarnIcon { text: trow.tempWhy }

      Label {
        text: trow.title
        font.pixelSize: 12
        color: brg.settings.textColorDark
        Layout.fillWidth: true
        wrapMode: Text.Wrap
      }

      MapSwitch {
        checked: trow.value
        onToggled: trow.toggle()
      }
    }

    Label {
      text: trow.blurb
      Layout.fillWidth: true
      wrapMode: Text.Wrap
      font.pixelSize: 10
      opacity: 0.55
    }
  }

  component ArmedNote: Label {
    Layout.fillWidth: true
    Layout.topMargin: 2
    wrapMode: Text.Wrap
    font.pixelSize: 10
    font.italic: true
    opacity: 0.5
  }

  // A small flat text pill -- the alike-group toolbar's verbs (View all / Check all / Uncheck all).
  // Same chassis idiom as MapClearButton: no 3-D bevel, accent-on-hover, pointing cursor.
  component MapTextButton: Item {
    id: tbtn
    property string text: ""
    signal clicked()

    implicitWidth: tpill.implicitWidth
    implicitHeight: 20

    Rectangle {
      id: tpill
      anchors.fill: parent
      implicitWidth: tlabel.implicitWidth + 14
      radius: height / 2
      color: thover.hovered ? "#2d79a7" : "transparent"
      border.width: 1
      border.color: thover.hovered ? "#2d79a7" : brg.settings.dividerColor
      Behavior on color { ColorAnimation { duration: 90 } }

      Label {
        id: tlabel
        anchors.centerIn: parent
        text: tbtn.text
        font.pixelSize: 10
        color: thover.hovered ? "#ffffff" : brg.settings.textColorMid
      }

      HoverHandler { id: thover; cursorShape: Qt.PointingHandCursor }
      TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: tbtn.clicked()
      }
    }
  }

  readonly property string tempWhy: qsTr("The game rewrites this every frame — it's forced back to "
    + "\"not over\" the instant you're not in the Safari Zone, so editing it here has no lasting "
    + "effect. Verified on a real cartridge. Shown because nothing is hidden.")

  // ── body ────────────────────────────────────────────────────────────────────────────────────
  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    // Reserve the 16px scrollbar lane (ui-patterns.md rule 2) so right-aligned fields don't slide
    // under the overlay bar.
    ColumnLayout {
      width: scroller.availableWidth - 16
      spacing: 6

      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 6

        Label {
          Layout.fillWidth: true
          text: qsTr("Stored values that belong to a specific map")
          wrapMode: Text.Wrap
          opacity: 0.6
          font.pixelSize: 11
        }

        // The map picker -- only maps that HAVE storage. Pre-selected to the current map when it has any.
        ComboBox {
          id: mapCombo
          Layout.fillWidth: true
          font.pixelSize: 12
          model: panel.storageMaps.map(function(m) { return m.title; })
          currentIndex: panel.page
          onActivated: function(i) { panel.page = i; }
        }

        // "You're here" / "you're elsewhere" -- so it's clear when an edit is live vs. dormant.
        Label {
          Layout.fillWidth: true
          wrapMode: Text.Wrap
          font.pixelSize: 10
          opacity: 0.55
          text: panel.onShownMap
                ? qsTr("You're on this map — edits here are live.")
                : qsTr("You're not on this map right now. The values are stored, but they only take "
                       + "effect once your save is here, mid-activity.")
        }

        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 2
          implicitHeight: 1
          color: brg.settings.dividerColor
        }

        // ── TOWN VISITED (top — Fairy Fox, 2026-07-17) ────────────────────────────────────────
        //
        // "have a visited checkbox near the top of persistent storage for places that take a visited
        // one." Only the 11 city maps take one (bit == map id; read only by Fly). It's an ALIKE
        // group: different bits, same kind of thing, one per place — so the header offers view-all
        // and check/uncheck-all across all eleven. @see notes/reference/town-visited.md
        ColumnLayout {
          id: townsSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageTowns(panel.curPage.ids) : [];
          }
          // This page's own town (0 or 1 of them) leads; "view all" reveals the whole group.
          readonly property var mine: list.length > 0 ? list[0] : undefined
          visible: mine !== undefined

          property bool showAll: false

          Label {
            text: qsTr("Town — visited")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: qsTr("Whether you've set foot here — the only thing that unlocks it as a Fly "
                       + "destination. It unlocks nothing else.")
          }

          // This map's own Visited switch.
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: townsSection.mine !== undefined

            // The one dynamic amber-! in the panel: the town you're SAVED IN re-marks itself on
            // Continue (outdoors), so clearing it here won't stick. Only this row, only this map.
            MapWarnIcon {
              visible: townsSection.mine !== undefined && townsSection.mine.isCurrentMap
              text: qsTr("You're standing in this town, so the game re-marks it visited the moment "
                         + "you Continue (it happens before the save's map protection). Clearing it "
                         + "sticks only if you save indoors or elsewhere. Verified on a cartridge.")
            }
            Label {
              text: townsSection.mine ? townsSection.mine.name : ""
              font.pixelSize: 12
              color: brg.settings.textColorDark
              Layout.fillWidth: true
              wrapMode: Text.Wrap
            }
            MapSwitch {
              checked: {
                panel.revision; panel.editTick;
                return (panel.wTowns && townsSection.mine)
                       ? panel.wTowns.townsAt(townsSection.mine.ind) : false;
              }
              onToggled: {
                if (panel.wTowns && townsSection.mine) {
                  panel.wTowns.townsSet(townsSection.mine.ind, checked);
                  panel.editTick++;
                }
              }
            }
          }

          // The ALIKE-group toolbar: view all eleven together, and check / uncheck the whole group.
          RowLayout {
            Layout.fillWidth: true
            spacing: 6

            MapTextButton {
              text: townsSection.showAll ? qsTr("Hide the other towns")
                                         : qsTr("View all 11 towns")
              onClicked: townsSection.showAll = !townsSection.showAll
            }
            Item { Layout.fillWidth: true }
            MapTextButton {
              text: qsTr("Check all")
              onClicked: {
                if (!panel.wTowns) return;
                for (var i = 0; i < 11; i++) panel.wTowns.townsSet(i, true);
                panel.editTick++;
              }
            }
            MapTextButton {
              text: qsTr("Uncheck all")
              onClicked: {
                if (!panel.wTowns) return;
                for (var i = 0; i < 11; i++) panel.wTowns.townsSet(i, false);
                panel.editTick++;
              }
            }
          }

          // View-all: every town, wherever you are — the "see the whole alike group at once"
          // affordance. Names come from storageTowns over all 11 ids (0..10 == the city map ids).
          readonly property var allTowns: {
            panel.revision;
            return townsSection.showAll
                   ? brg.map.storageTowns([0,1,2,3,4,5,6,7,8,9,10]) : [];
          }
          Repeater {
            model: townsSection.allTowns
            delegate: RowLayout {
              id: townAll
              required property var modelData
              Layout.fillWidth: true
              Layout.leftMargin: 8
              spacing: 6

              Label {
                text: townAll.modelData.name
                font.pixelSize: 11
                opacity: 0.85
                Layout.fillWidth: true
                elide: Text.ElideRight
              }
              MapSwitch {
                checked: {
                  panel.revision; panel.editTick;
                  return panel.wTowns ? panel.wTowns.townsAt(townAll.modelData.ind) : false;
                }
                onToggled: {
                  if (panel.wTowns) { panel.wTowns.townsSet(townAll.modelData.ind, checked); panel.editTick++; }
                }
              }
            }
          }

          Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 2
            implicitHeight: 1
            color: brg.settings.dividerColor
          }
        }

        // ── THE MAP'S SCRIPT (top — Twilight, 2026-07-16) ─────────────────────────────────────
        // The per-map script-progress value (w<Map>CurScript). The dropdown reads like a story:
        // each named step describes what's happening at that stage of the map's events.
        ColumnLayout {
          id: scriptSection
          Layout.fillWidth: true
          spacing: 4
          visible: panel.curPage !== undefined && panel.curPage.scriptInd >= 0

          readonly property int scriptInd: panel.curPage !== undefined ? panel.curPage.scriptInd : -1
          readonly property int stepCount: panel.curPage !== undefined ? panel.curPage.steps : 0
          readonly property var steps: {
            panel.revision;
            return scriptInd >= 0 ? brg.map.storageScriptSteps(scriptInd) : [];
          }
          readonly property int value: {
            panel.revision; panel.editTick;
            return panel.wScripts && scriptInd >= 0 ? panel.wScripts.scriptsAt(scriptInd) : 0;
          }
          readonly property bool valueIsNamed: {
            for (let i = 0; i < steps.length; i++)
              if (steps[i].value === value) return true;
            return false;
          }
          property bool customMode: false

          Label {
            // "Map STATE", not "map script" (leadership, 2026-07-17): the byte is one field of
            // the map's progression state, and the state is what a person is editing here.
            text: qsTr("Map state")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: panel.curPage !== undefined && panel.curPage.desc ? panel.curPage.desc : ""
            visible: text !== ""
          }

          // Which option matches the stored value ("Something else…" = the last slot).
          readonly property int comboIndex: {
            panel.revision; panel.editTick;
            if (customMode || !valueIsNamed)
              return steps.length;
            for (let i = 0; i < steps.length; i++)
              if (steps[i].value === value) return i;
            return steps.length;
          }

          // The dropdown: the named steps + "Something else…" for a custom value.
          ComboBox {
            id: stepCombo
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            font.pixelSize: 12
            visible: scriptSection.steps.length > 0
            textRole: "name"
            model: {
              const l = scriptSection.steps.slice();
              l.push({ value: -1, name: qsTr("Something else…"), desc: "" });
              return l;
            }
            onActivated: function(i) {
              if (i === model.length - 1) {           // "Something else…"
                scriptSection.customMode = true;
                return;
              }
              scriptSection.customMode = false;
              if (panel.wScripts) {
                panel.wScripts.scriptsSet(scriptSection.scriptInd, scriptSection.steps[i].value);
                panel.editTick++;
              }
            }
            // A model change (page switch) makes the ComboBox clamp currentIndex
            // itself — re-assert the stored step's index AFTER the new model lands.
            onModelChanged: currentIndex = scriptSection.comboIndex
          }
          // A Binding element, NOT a plain currentIndex binding: the combo's own
          // internal writes would sever a plain binding. `delayed` coalesces to the
          // end of the event loop, after the model rebuild — otherwise the index can
          // be applied to the OLD model and clamped (the Oak's-Lab-shows-"Default"
          // bug the screenshot review caught, twice).
          Binding {
            target: stepCombo
            property: "currentIndex"
            value: scriptSection.comboIndex
            delayed: true
          }

          // The selected step's progression description — the sense of WHERE the story is.
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            visible: stepCombo.visible && !scriptSection.customMode && text !== ""
            text: {
              panel.revision; panel.editTick;
              for (let i = 0; i < scriptSection.steps.length; i++)
                if (scriptSection.steps[i].value === scriptSection.value)
                  return scriptSection.steps[i].desc !== undefined ? scriptSection.steps[i].desc : "";
              return "";
            }
          }

          // The custom path: full byte range, never refused — warned when it's past the map's
          // own table (the unbounded jp-hl dispatch; same crash mechanism the event-flag research
          // documented).
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: scriptSection.steps.length === 0 || scriptSection.customMode
                     || !scriptSection.valueIsNamed

            Label {
              text: qsTr("Custom step value")
              font.pixelSize: 11
              color: brg.settings.textColorMid
              Layout.fillWidth: true
            }
            TextField {
              Layout.preferredWidth: 64
              font.pixelSize: 12
              horizontalAlignment: TextInput.AlignRight
              text: scriptSection.value
              inputMethodHints: Qt.ImhDigitsOnly
              validator: IntValidator { bottom: 0; top: 255 }
              onEditingFinished: {
                const v = parseInt(text);
                if (!isNaN(v) && panel.wScripts) {
                  panel.wScripts.scriptsSet(scriptSection.scriptInd,
                                            Math.max(0, Math.min(255, v)));
                  panel.editTick++;
                }
              }
            }
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            color: "#d55e00"
            visible: scriptSection.stepCount > 0 && scriptSection.value >= scriptSection.stepCount
            text: qsTr("⚠ Step %1 is beyond this map's state table (0–%2). The game dispatches "
                       + "state steps through an unbounded pointer table — an out-of-range step "
                       + "makes it jump to garbage, a real crash risk. Stored as asked, never "
                       + "rewritten.")
                  .arg(scriptSection.value).arg(scriptSection.stepCount - 1)
          }
          ArmedNote {
            visible: scriptSection.visible
            text: qsTr("Durable in the save. It steers this map's state and scripted events the "
                       + "moment your save is there.")
          }

          Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 2
            implicitHeight: 1
            color: brg.settings.dividerColor
            visible: (panel.curPage !== undefined && panel.curPage.legacy >= 0)
                     || missableSection.list.length > 0
          }
        }

        // ── Vermilion Gym ─────────────────────────────────────────────────────────────────────
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 6
          visible: panel.curPage !== undefined && panel.curPage.legacy === 0

          NumRow {
            title: qsTr("Trash-can switch 1")
            blurb: qsTr("Which trash can holds the FIRST hidden switch (0–14, even). Randomised when "
                        + "you enter the gym.")
            value: panel.local ? panel.local.lock1 : 0
            onSetValue: function(v) { if (panel.local) panel.local.lock1 = v; }
          }
          NumRow {
            title: qsTr("Trash-can switch 2")
            blurb: qsTr("Which trash can holds the SECOND switch (0–14). Chosen once you flip the "
                        + "first correctly.")
            value: panel.local ? panel.local.lock2 : 0
            onSetValue: function(v) { if (panel.local) panel.local.lock2 = v; }
          }
          ArmedNote {
            text: qsTr("Takes effect while you're mid-puzzle in Vermilion Gym.")
          }
        }

        // ── Cinnabar Gym ──────────────────────────────────────────────────────────────────────
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 6
          visible: panel.curPage !== undefined && panel.curPage.legacy === 1

          NumRow {
            title: qsTr("Next wrong-answer opponent")
            blurb: qsTr("The trainer you battle if you get the NEXT quiz-gate answer wrong (a sprite "
                        + "index; 0 = none queued). Set fresh as you work through the gates.")
            value: panel.local ? panel.local.quizOpp : 0
            onSetValue: function(v) { if (panel.local) panel.local.quizOpp = v; }
          }
          ArmedNote {
            text: qsTr("Takes effect while you're mid-quiz in Cinnabar Gym.")
          }
        }

        // ── Safari Zone (combined) ────────────────────────────────────────────────────────────
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 6
          visible: panel.curPage !== undefined && panel.curPage.legacy === 2

          NumRow {
            title: qsTr("Steps left")
            blurb: qsTr("Steps remaining in this Safari run (a real run starts at 502). Reset to 502 "
                        + "when you enter through the gate.")
            value: panel.local ? panel.local.safariSteps : 0
            maxStorable: 65535     // a 2-byte counter (big-endian in the save)
            onSetValue: function(v) { if (panel.local) panel.local.safariSteps = v; }
          }
          NumRow {
            title: qsTr("Safari Balls")
            blurb: qsTr("Balls remaining (a run starts with 30). Reset to 30 on entry, 0 on the way out.")
            value: panel.local ? panel.local.safariBallCount : 0
            onSetValue: function(v) { if (panel.local) panel.local.safariBallCount = v; }
          }

          TempFlagRow {
            title: qsTr("Game over")
            blurb: qsTr("Marks the current Safari run as ended.")
            tempWhy: panel.tempWhy
            value: panel.local ? panel.local.safariGameOver : false
            onToggle: function() { if (panel.local) panel.local.safariGameOver = !panel.local.safariGameOver; }
          }

          ArmedNote {
            text: qsTr("Steps and balls take effect only while your save is inside the Safari Zone; "
                       + "the gate resets them to 502 / 30 on entry.")
          }
        }

        // ── EVENT FLAGS — what has HAPPENED here ───────────────────────────────────────────────
        // The story bits wEventFlags keeps for this place: ONE contiguous 320-byte field at save
        // 0x29F3 (bit `ind` = byte 0x29F3 + ind/8, bit ind%8), console-verified live on Continue.
        // Every flag is filed on its OWN map; one that spans several (Silph Co's bits across its
        // 12 floors) is SHARED and shows on each of their pages in a labelled shared group naming
        // the others. Groups are open -- this is a page you scroll (leadership 2026-07-16).
        // Reference: notes/reference/event-flags.md.
        ColumnLayout {
          id: eventSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageEvents(panel.curPage.ids) : [];
          }
          visible: list.length > 0

          /// Group name -> its rows, real groups first, SHARED next, Placeholder Flags LAST.
          readonly property var groups: {
            const l = eventSection.list;
            let byName = ({});
            for (let i = 0; i < l.length; i++) {
              const e = l[i];
              const key = e.placeholder ? qsTr("Placeholder Flags")
                        : (e.shared ? qsTr("Shared · Event flags · %1").arg(e.sharedWith.join(", "))
                                    : (e.group !== "" ? e.group : qsTr("Story")));
              if (byName[key] === undefined) byName[key] = [];
              byName[key].push(e);
            }
            let keys = Object.keys(byName);
            keys.sort(function(a, b) {
              const rank = function(k) {
                if (k === qsTr("Placeholder Flags")) return 2;
                if (k.indexOf(qsTr("Shared ·")) === 0) return 1;
                return 0;
              };
              const ra = rank(a), rb = rank(b);
              return ra !== rb ? ra - rb : a.localeCompare(b);
            });
            return keys.map(function(k) { return { title: k, rows: byName[k] }; });
          }

          Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: brg.settings.dividerColor
            visible: panel.curPage !== undefined
                     && (panel.curPage.scriptInd >= 0 || panel.curPage.legacy >= 0)
          }

          Label {
            text: qsTr("Event flags — what's happened here")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: qsTr("Each switch is one thing the game remembers about this place — a trainer "
                       + "beaten, an item taken, a scene watched. ON = it has happened.")
          }

          Repeater {
            model: eventSection.groups

            delegate: ColumnLayout {
              id: grp
              required property var modelData
              Layout.fillWidth: true
              spacing: 3

              readonly property bool isShared: modelData.title.indexOf(qsTr("Shared ·")) === 0
              readonly property bool isPlaceholder: modelData.title === qsTr("Placeholder Flags")

              Item { Layout.preferredHeight: 4 }

              RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                  text: grp.modelData.title
                  font.pixelSize: 11
                  font.bold: true
                  color: grp.isShared ? "#3b6ea5" : brg.settings.textColorDark
                  opacity: grp.isPlaceholder ? 0.6 : 1.0
                  Layout.fillWidth: true
                  wrapMode: Text.Wrap
                }

                // The group toggle: sets/clears every flag in the group at once.
                MapSwitch {
                  checked: {
                    panel.revision; panel.editTick;
                    if (!panel.wEvents) return false;
                    for (let i = 0; i < grp.modelData.rows.length; i++)
                      if (!panel.wEvents.eventsAt(grp.modelData.rows[i].ind)) return false;
                    return true;
                  }
                  onToggled: {
                    if (!panel.wEvents) return;
                    let all = true;
                    for (let i = 0; i < grp.modelData.rows.length; i++)
                      if (!panel.wEvents.eventsAt(grp.modelData.rows[i].ind)) { all = false; break; }
                    for (let j = 0; j < grp.modelData.rows.length; j++)
                      panel.wEvents.eventsSet(grp.modelData.rows[j].ind, !all);
                    panel.editTick++;
                  }
                }
              }

              // Shared groups say plainly what they are and where else they live.
              Label {
                visible: grp.isShared
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 10
                color: "#3b6ea5"
                text: qsTr("Shared with other maps — the same save bits appear on those pages too, "
                           + "so a change here shows up there.")
              }
              Label {
                visible: grp.isPlaceholder
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 10
                opacity: 0.55
                text: qsTr("Spare bits the game never reads. Editable like anything else — but "
                           + "nothing in the game looks at them.")
              }

              Repeater {
                model: grp.modelData.rows

                delegate: ColumnLayout {
                  id: erow
                  required property var modelData
                  Layout.fillWidth: true
                  spacing: 1

                  /// The research classification, in words a person can act on. "used"
                  /// is the default (500+ rows) and "placeholder" is the group's whole
                  /// title, so neither earns a chip.
                  readonly property var badges: {
                    let out = [];
                    const c = erow.modelData.classification;
                    for (let i = 0; i < c.length; i++) {
                      if (c[i] === "temporary")    out.push(qsTr("temporary"));
                      else if (c[i] === "vestigial")   out.push(qsTr("does nothing"));
                      else if (c[i] === "defined-unused") out.push(qsTr("never used"));
                      else if (c[i] === "block-swept") out.push(qsTr("swept in a group"));
                    }
                    return out;
                  }

                  RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    MapWarnIcon {
                      visible: erow.modelData.caution !== ""
                      text: erow.modelData.caution
                    }

                    Label {
                      text: erow.modelData.name
                      font.pixelSize: 12
                      color: brg.settings.textColorDark
                      opacity: erow.modelData.placeholder ? 0.7 : 1.0
                      Layout.fillWidth: true
                      wrapMode: Text.Wrap
                    }

                    MapSwitch {
                      checked: {
                        panel.revision; panel.editTick;
                        return panel.wEvents ? panel.wEvents.eventsAt(erow.modelData.ind) : false;
                      }
                      onToggled: {
                        if (panel.wEvents) {
                          panel.wEvents.eventsSet(erow.modelData.ind,
                                                  !panel.wEvents.eventsAt(erow.modelData.ind));
                          panel.editTick++;
                        }
                      }
                    }
                  }

                  Label {
                    text: erow.modelData.desc
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    font.pixelSize: 10
                    opacity: 0.55
                    visible: text !== "" && !erow.modelData.placeholder
                  }

                  // What the research found this flag IS, plus where the bit lives.
                  // Only the classifications a person can act on are shown -- "used"
                  // is the default and saying it on 500 rows would be noise.
                  RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: erow.badges.length > 0 || !erow.modelData.placeholder

                    Repeater {
                      model: erow.badges
                      delegate: Rectangle {
                        required property string modelData
                        radius: 3
                        color: modelData === qsTr("temporary") ? "#b07d10" : "#6b7280"
                        opacity: 0.85
                        implicitWidth: bl.implicitWidth + 8
                        implicitHeight: bl.implicitHeight + 3
                        Label {
                          id: bl
                          anchors.centerIn: parent
                          text: parent.modelData
                          font.pixelSize: 8
                          color: "#ffffff"
                        }
                      }
                    }

                    Item { Layout.fillWidth: true }

                    Label {   // the raw path -- never hidden from a power user
                      text: qsTr("byte 0x%1 · bit %2")
                              .arg(erow.modelData.byte.toString(16).toUpperCase())
                              .arg(erow.modelData.bit)
                      font.pixelSize: 8
                      font.family: "monospace"
                      opacity: 0.35
                    }
                  }
                }
              }
            }
          }
        }

        // ── HIDDEN ITEMS & COINS — what's buried here ──────────────────────────────────────────
        //
        // The 54 hidden items and 12 coin piles: the ones you only find with the Itemfinder, or by
        // pressing A at exactly the right patch of nothing. Each one's save bit IS its row in the
        // cartridge's own coord table, so every switch here is an exact (map, x, y) -- the cleanest
        // positional data in the game.
        //
        // ⚠️ The word is COLLECTED, not hidden. The bit means "you already picked this up", which is
        // the opposite direction from a Filter Flag's bit -- and borrowing the missable's wording
        // here would invert the meaning of every switch on the page.
        ColumnLayout {
          id: hiddenSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageHidden(panel.curPage.ids) : [];
          }
          visible: list.length > 0

          Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: brg.settings.dividerColor
          }

          Label {
            text: qsTr("Hidden Items & Coins — what's buried here")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: qsTr("Nothing marks these on the map — you find them with the Itemfinder, or by "
                       + "chance. ON = you've already picked it up, so the spot is now empty.")
          }

          // Two ALIKE groups, kept separate (leadership, 2026-07-17: "hidden items and coins another
          // group id keep them 2 separate groups") — items and coins are different save arrays with
          // independent numbering. Check/uncheck acts on the WHOLE group (all 54 / all 12, every
          // map), so the labels name the full counts to make that scope plain.
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
              text: qsTr("All 54 items:")
              font.pixelSize: 10
              opacity: 0.6
              Layout.fillWidth: true
            }
            MapTextButton {
              text: qsTr("Check all")
              onClicked: {
                if (!panel.wHidden) return;
                for (var i = 0; i < panel.wHidden.hItemsCount(); i++) panel.wHidden.hItemsSet(i, true);
                panel.editTick++;
              }
            }
            MapTextButton {
              text: qsTr("Uncheck all")
              onClicked: {
                if (!panel.wHidden) return;
                for (var i = 0; i < panel.wHidden.hItemsCount(); i++) panel.wHidden.hItemsSet(i, false);
                panel.editTick++;
              }
            }
          }
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
              text: qsTr("All 12 coin piles:")
              font.pixelSize: 10
              opacity: 0.6
              Layout.fillWidth: true
            }
            MapTextButton {
              text: qsTr("Check all")
              onClicked: {
                if (!panel.wHidden) return;
                for (var i = 0; i < panel.wHidden.hCoinsCount(); i++) panel.wHidden.hCoinsSet(i, true);
                panel.editTick++;
              }
            }
            MapTextButton {
              text: qsTr("Uncheck all")
              onClicked: {
                if (!panel.wHidden) return;
                for (var i = 0; i < panel.wHidden.hCoinsCount(); i++) panel.wHidden.hCoinsSet(i, false);
                panel.editTick++;
              }
            }
          }

          Repeater {
            model: hiddenSection.list

            delegate: RowLayout {
              id: hrow
              required property var modelData
              Layout.fillWidth: true
              spacing: 6

              readonly property bool collected: {
                panel.revision; panel.editTick;
                return panel.wHidden
                    ? (modelData.isCoin ? panel.wHidden.hCoinsAt(modelData.ind)
                                        : panel.wHidden.hItemsAt(modelData.ind))
                    : false;
              }

              /// Arrived here by clicking this pickup's box on the map.
              readonly property bool highlighted: panel.highlightSection === "hidden"
                                                  && panel.highlightInd === hrow.modelData.ind

              Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: inner.implicitHeight + 6
                // The "here it is" wash, in the same ink the pickup's own box is drawn in, so the
                // box you clicked and the row you land on are visibly the same thing.
                color: hrow.highlighted
                         ? (hrow.modelData.isCoin ? "#f0e44222" : "#cc79a722")
                         : "transparent"
                radius: 3
                Behavior on color { ColorAnimation { duration: 180 } }

                RowLayout {
                  id: inner
                  anchors.fill: parent
                  anchors.margins: 3
                  spacing: 6

                  // ⚠️ MapSwitch, NOT FlatToggle. FlatToggle is not a type this file can see, and an
                  // unresolved type does not fail politely: the WHOLE panel body silently refuses to
                  // instantiate and Map Storage opens completely BLANK -- on every map, not just the
                  // one with the mistake. It shipped past tst_qml_screens because the panel lives in
                  // a dock Loader that only builds when the dock is opened, and the smoke test never
                  // opens it. Found by Twilight in seconds. See notes/reference/qt-patterns.md.
                  MapSwitch {
                    checked: hrow.collected
                    onToggled: {
                      if (!panel.wHidden)
                        return;
                      if (hrow.modelData.isCoin)
                        panel.wHidden.hCoinsSet(hrow.modelData.ind, checked);
                      else
                        panel.wHidden.hItemsSet(hrow.modelData.ind, checked);
                      panel.editTick++;
                    }
                  }

                  ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                      Layout.fillWidth: true
                      text: hrow.modelData.name
                      font.pixelSize: 11
                      color: brg.settings.textColorDark
                      elide: Text.ElideRight
                    }
                    Label {
                      Layout.fillWidth: true
                      // The coordinates are the point of this list: it is the only storage kind
                      // whose bit IS a place, exactly.
                      text: qsTr("(%1, %2) · %3")
                              .arg(hrow.modelData.x).arg(hrow.modelData.y)
                              .arg(hrow.collected ? qsTr("collected") : qsTr("still there"))
                      font.pixelSize: 9
                      opacity: 0.5
                      elide: Text.ElideRight
                    }
                  }
                }
              }
            }
          }
        }

        // ── FILTER FLAGS — who's on this map at all ────────────────────────────────────────────
        // The map's hide/show object bits (wToggleableObjectFlags, save 0x2852; bit SET = HIDDEN,
        // so the switch shows the intuitive direction: checked = on the map). "Filter Flags" is
        // Twilight's preferred term (2026-07-16, cased as a proper name) for what the game/pret
        // calls missables — the model classes keep the pret name; the UI says Filter Flags.
        // Sorted into its own group;
        // pret's known-issue oddballs wear an amber note; linked event flags surfaced live.
        ColumnLayout {
          id: missableSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageMissables(panel.curPage.ids) : [];
          }
          visible: list.length > 0

          Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: brg.settings.dividerColor
            visible: panel.curPage !== undefined
                     && (panel.curPage.scriptInd >= 0 || panel.curPage.legacy >= 0)
          }

          Label {
            text: qsTr("Filter Flags — who's on the map")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: qsTr("Each switch is one person, item ball or Pokémon this map can hide or show. "
                       + "ON = it's there. The game flips these as the story advances.")
          }

          Repeater {
            model: missableSection.list

            delegate: ColumnLayout {
              id: mrow
              required property var modelData
              Layout.fillWidth: true
              spacing: 1

              readonly property bool hidden: {
                panel.revision; panel.editTick;
                return panel.wMissables ? panel.wMissables.missablesAt(modelData.ind) : false;
              }

              /// Arrived here by clicking this thing's box on the map. @see panel.revealMissable
              readonly property bool highlighted: panel.highlightMissable === mrow.modelData.ind

              // The "here it is" wash. The Flag boxes layer's own bluish green (#009e73), so the box
              // you clicked and the row you land on are visibly the same colour -- the canvas and the
              // panel saying the same thing. It fades itself out; a highlight that stayed would just
              // become another thing to dismiss.
              Rectangle {
                z: -1
                anchors.fill: parent
                anchors.margins: -3
                radius: 3
                visible: mrow.highlighted
                color: "#26009e73"
                border.width: 1
                border.color: "#009e73"
              }

              RowLayout {
                Layout.fillWidth: true
                spacing: 6

                MapWarnIcon {
                  visible: mrow.modelData.oddity !== ""
                  text: qsTr("pret's own note on this one: %1. The bit exists and is stored, but "
                             + "toggling it does nothing visible.").arg(mrow.modelData.oddity)
                }

                Label {
                  text: mrow.modelData.name
                  font.pixelSize: 12
                  color: brg.settings.textColorDark
                  Layout.fillWidth: true
                  wrapMode: Text.Wrap
                }

                MapSwitch {
                  checked: !mrow.hidden          // bit SET = HIDDEN; the switch says "on the map"
                  onToggled: {
                    if (panel.wMissables) {
                      panel.wMissables.missablesSet(mrow.modelData.ind, mrow.hidden ? false : true);
                      panel.editTick++;
                    }
                  }
                }
              }

              Label {
                text: mrow.modelData.desc
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 10
                opacity: 0.55
                visible: text !== ""
              }

              // The conflict hooks: the event flags the game consults around this object,
              // with their LIVE state — so a contradiction is visible at a glance.
              Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 10
                color: "#b07d10"
                visible: mrow.modelData.linked.length > 0
                text: {
                  panel.revision; panel.editTick;
                  let parts = [];
                  for (let i = 0; i < mrow.modelData.linked.length; i++) {
                    const l = mrow.modelData.linked[i];
                    let state = qsTr("not modelled");
                    if (l.eventIndex >= 0 && panel.wEvents)
                      state = panel.wEvents.eventsAt(l.eventIndex) ? qsTr("ON") : qsTr("off");
                    parts.push(l.flag + " (" + state + ")");
                  }
                  return qsTr("Tied to %1 — the game toggles this object around that flag. A flag "
                              + "and a switch that disagree is untested territory.").arg(parts.join(", "));
                }
              }
            }
          }
        }

        // ── IN-GAME TRADES — the NPC swaps here (Fairy Fox, 2026-07-17) ────────────────────────
        //
        // The ten NPC trades, on their trader's tile. An ALIKE group (different bits, same kind) —
        // so the header offers check/uncheck-all across all ten. The unused CHIKUCHIKU trade lands
        // on the General page (no map). ⚠️ The name shown is the RECEIVED mon's nickname, and the
        // trader has only a class name. @see notes/reference/in-game-trades.md
        ColumnLayout {
          id: tradesSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageTrades(panel.curPage.ids) : [];
          }
          visible: list.length > 0

          readonly property var dialogsetNames: [qsTr("casual"), qsTr("evolution"), qsTr("happy")]

          Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: brg.settings.dividerColor
          }

          Label {
            text: qsTr("In-game Trades — the NPC swaps here")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: qsTr("ON = you've already done this trade, so the trader won't deal again. "
                       + "Clearing it re-arms the swap.")
          }

          // Alike-group toolbar: the whole set of ten, check or clear together.
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
              text: qsTr("All 10 trades:")
              font.pixelSize: 10
              opacity: 0.6
              Layout.fillWidth: true
            }
            MapTextButton {
              text: qsTr("Check all")
              onClicked: {
                if (!panel.wTrades) return;
                for (var i = 0; i < 10; i++) panel.wTrades.tradesSet(i, true);
                panel.editTick++;
              }
            }
            MapTextButton {
              text: qsTr("Uncheck all")
              onClicked: {
                if (!panel.wTrades) return;
                for (var i = 0; i < 10; i++) panel.wTrades.tradesSet(i, false);
                panel.editTick++;
              }
            }
          }

          Repeater {
            model: tradesSection.list
            delegate: RowLayout {
              id: trd
              required property var modelData
              Layout.fillWidth: true
              spacing: 6

              MapSwitch {
                checked: {
                  panel.revision; panel.editTick;
                  return panel.wTrades ? panel.wTrades.tradesAt(trd.modelData.ind) : false;
                }
                onToggled: {
                  if (panel.wTrades) { panel.wTrades.tradesSet(trd.modelData.ind, checked); panel.editTick++; }
                }
              }

              ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Label {
                  Layout.fillWidth: true
                  // The received mon's nickname is the memorable handle; the swap follows.
                  text: qsTr("%1 — %2 → %3")
                          .arg(trd.modelData.nickname).arg(trd.modelData.give).arg(trd.modelData.get)
                  font.pixelSize: 11
                  color: brg.settings.textColorDark
                  elide: Text.ElideRight
                }
                Label {
                  Layout.fillWidth: true
                  text: {
                    // Trader class + coords (or "unused" on the General page). The dialog-set tag
                    // rides along; "evolution" is the localisation scar, not a real evolution.
                    const d = tradesSection.dialogsetNames[trd.modelData.dialogset] || "";
                    if (trd.modelData.unused)
                      return qsTr("unused — never reachable in-game · %1").arg(d);
                    return qsTr("%1 at (%2, %3)%4 · %5")
                             .arg(trd.modelData.trader).arg(trd.modelData.x).arg(trd.modelData.y)
                             .arg(trd.modelData.walks ? qsTr(" (wanders)") : "").arg(d);
                  }
                  font.pixelSize: 9
                  opacity: 0.5
                  elide: Text.ElideRight
                }
              }
            }
          }
        }

        // ── COMPLETED ONE-SHOTS — rods, Lapras, starter, nurse, guards, Elite 4 ────────────────
        //
        // @see notes/reference/world-completed.md. Rods are an ALIKE group; guards/starter/nurse are
        // SHARED (one bit, several maps). startedElite4 is the loaded gun — its caution says so.
        ColumnLayout {
          id: completedSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageCompleted(panel.curPage.ids) : [];
          }
          visible: list.length > 0

          readonly property bool hasRods: {
            for (var i = 0; i < list.length; i++) if (list[i].group === "rods") return true;
            return false;
          }

          Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: brg.settings.dividerColor
          }

          Label {
            text: qsTr("Milestones — one-time events here")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }

          // The rods are an alike group of three: check/clear together.
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: completedSection.hasRods
            Label {
              text: qsTr("All 3 rods:")
              font.pixelSize: 10
              opacity: 0.6
              Layout.fillWidth: true
            }
            MapTextButton {
              text: qsTr("Check all")
              onClicked: {
                if (!panel.wCompleted) return;
                panel.wCompleted.obtainedOldRod = true;
                panel.wCompleted.obtainedGoodRod = true;
                panel.wCompleted.obtainedSuperRod = true;
                panel.editTick++;
              }
            }
            MapTextButton {
              text: qsTr("Uncheck all")
              onClicked: {
                if (!panel.wCompleted) return;
                panel.wCompleted.obtainedOldRod = false;
                panel.wCompleted.obtainedGoodRod = false;
                panel.wCompleted.obtainedSuperRod = false;
                panel.editTick++;
              }
            }
          }

          Repeater {
            model: completedSection.list
            delegate: ColumnLayout {
              id: cmp
              required property var modelData
              Layout.fillWidth: true
              spacing: 1

              RowLayout {
                Layout.fillWidth: true
                spacing: 6

                MapWarnIcon {
                  visible: cmp.modelData.caution !== ""
                  text: cmp.modelData.caution
                }
                Label {
                  text: cmp.modelData.name
                  font.pixelSize: 12
                  color: brg.settings.textColorDark
                  Layout.fillWidth: true
                  wrapMode: Text.Wrap
                }
                MapSwitch {
                  checked: {
                    panel.revision; panel.editTick;
                    return panel.wCompleted ? panel.wCompleted[cmp.modelData.key] === true : false;
                  }
                  onToggled: {
                    if (panel.wCompleted) { panel.wCompleted[cmp.modelData.key] = checked; panel.editTick++; }
                  }
                }
              }
              Label {
                Layout.fillWidth: true
                text: cmp.modelData.desc
                wrapMode: Text.Wrap
                font.pixelSize: 10
                opacity: 0.55
              }
              // A shared flag: one bit, several maps — name the others so it's clear editing here
              // changes them all.
              Label {
                Layout.fillWidth: true
                visible: cmp.modelData.groupKind === "shared" && cmp.modelData.mapIds.length > 1
                text: qsTr("Shared — one flag across %1 places.").arg(cmp.modelData.mapIds.length)
                wrapMode: Text.Wrap
                font.pixelSize: 9
                opacity: 0.45
              }
            }
          }
        }

        // ── FOSSIL — what you gave the lab, and what it becomes ────────────────────────────────
        //
        // Cinnabar Lab Fossil Room only. ⚠️ The two bytes are INDEPENDENT (console-proven): the
        // resulting Pokémon is what the machine hands over, the fossil item only names the line.
        // We show both and sync NEITHER, and never warn on a mismatched pair (resting state).
        // @see notes/reference/fossil-revival.md
        ColumnLayout {
          id: fossilSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageFossil(panel.curPage.ids) : [];
          }
          visible: list.length > 0

          Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: brg.settings.dividerColor
          }

          Label {
            text: qsTr("Fossil — given & revived")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }

          Repeater {
            model: fossilSection.list
            delegate: NumRow {
              required property var modelData
              title: modelData.name
              blurb: modelData.desc
              value: {
                panel.revision; panel.editTick;
                return panel.wOther ? panel.wOther[modelData.key] : 0;
              }
              maxStorable: 255
              onSetValue: function(v) {
                if (panel.wOther) { panel.wOther[modelData.key] = v; panel.editTick++; }
              }
            }
          }
        }

        Item { Layout.preferredHeight: 12 }   // breathing room at the bottom of the scroll
      }
    }
  }
}
