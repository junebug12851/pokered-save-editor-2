import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../general"
import "../../header"
import "../../controls/selection"

// OverviewTab.qml -- the "General" tab of the Pokemon details editor.
//
// Bound to boxData (a PokemonBox). The fields (nickname, type 1/2, OT name+ID, exp,
// catch rate, future nature) live in one bordered panel as a cohesive list of
// zebra-striped rows (the Bag/Market grouping applied to a form), each [muted label
// | control(s) | connected action group]. The action groups are connected
// "combo" segments (randomize / clear) right-aligned in the panel, clear of the
// scrollbar. Type clear resets to the species' DB-default type(s); type randomize
// rolls both. OT actions are top-aligned with the OT-Name row. Closes the screen
// if the underlying file data changes (a new file was loaded).
Rectangle {
  id: top
  color: "transparent"

  property PokemonBox boxData: null

  // One height knob per control type so rows stay consistently sized despite
  // Material text fields and combos having different natural heights.
  property int textH: 30   // text boxes (shorter)
  property int comboH: 38  // combos (a touch taller)

  // Zebra tint laid over the white panel for alternate rows.
  property color rowAlt: Qt.rgba(0, 0, 0, 0.04)

  // Roll both types to random valid ones (via the type combos, which carry the
  // type-id value role). onActivated only fires on user input, so write boxData too.
  function randomizeTypes() {
    type1.currentIndex = Math.floor(Math.random() * type1.count);
    boxData.type1 = type1.currentValue;
    theType2.currentIndex = Math.floor(Math.random() * theType2.count);
    boxData.type2 = theType2.currentValue;
  }

  // Roll a random (future) nature via the nature combo. setNature rewrites the DVs
  // to produce it (same path the combo's onActivated uses).
  function randomizeNature() {
    futureNatureEdit.currentIndex = Math.floor(Math.random() * futureNatureEdit.count);
    boxData.setNature(futureNatureEdit.currentValue);
  }

  // Close details screen if the file data changes
  Connections {
    target: brg.file.data
    function onDataExpandedChanged() { brg.router.closeScreen(); }
  }

  // Muted, right-aligned row label (replaces the old shaded grey label box). Fixed
  // width so every row's label lines up in one column.
  component RowLabel: Text {
    Layout.preferredWidth: 90
    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    horizontalAlignment: Text.AlignRight
    verticalAlignment: Text.AlignVCenter
    color: brg.settings.textColorMid
    font.pixelSize: 13
    elide: Text.ElideRight
  }

  // A single segment of a connected action-button "combo" group. Flat, icon-only,
  // with a square hover/press fill and a hairline left divider (except the first),
  // filling the group's height. Tinted like the app's other icons.
  component SegBtn: Button {
    id: seg
    property bool first: false
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
      // -16 reserves the Material scrollbar lane; the panel margin then keeps the
      // grouped rows (and their right-aligned action groups) well clear of the
      // overlay scrollbar (see ui-patterns.md -> "Scrollable forms").
      width: scroller.availableWidth - 16
      spacing: 0

      // The single grouped panel that holds every field row (Bag/Market style).
      Rectangle {
        Layout.fillWidth: true
        implicitHeight: rowsCol.implicitHeight
        color: brg.settings.textColorLight
        clip: true

        ColumnLayout {
          id: rowsCol
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          spacing: 0

          // ---- Nickname ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              spacing: 12

              RowLabel { id: nickLbl; text: "Nickname" }

              NameDisplay {
                id: monNameEdit
                Layout.alignment: Qt.AlignVCenter

                isPersonName: false
                isPlayerName: false
                sizeMult: nickLbl.font.pixelSize / 8

                onStrChanged: boxData.nickname = str;
                Connections {
                  target: boxData
                  function onNicknameChanged() { monNameEdit.str = boxData.nickname; }
                }
                Component.onCompleted: monNameEdit.str = boxData.nickname;
              }

              Item { Layout.fillWidth: true }

              // Randomize / Clear the nickname. changeName(false) randomizes;
              // changeName(true) removes it (reverts to the species name).
              Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 30
                implicitWidth: nickGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: nickGrp
                  anchors.fill: parent
                  spacing: 0
                  SegBtn {
                    first: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
                    onClicked: boxData.changeName(false);
                    tip: qsTr("Give the Pokémon a random nickname. (A name counts as a nickname when it isn't the species name in all caps; nicknames are unaffected by evolution.)")
                  }
                  SegBtn {
                    icon.width: 16; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
                    enabled: boxData.hasNickname
                    onClicked: boxData.changeName(true);
                    tip: qsTr("Remove the nickname (revert to the species name in caps).")
                  }
                }
              }
            }
          }

          // ---- Type ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: top.rowAlt

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              spacing: 12

              RowLabel { text: "Type" }

              SelectType {
                id: type1
                Layout.alignment: Qt.AlignVCenter
                // Cap at the comfortable width but allow shrinking so the action
                // group always stays inside the panel on a narrow pane.
                Layout.fillWidth: true
                Layout.maximumWidth: font.pixelSize * 8
                Layout.minimumWidth: font.pixelSize * 4
                Layout.preferredHeight: top.comboH

                onActivated: boxData.type1 = currentValue;
                Component.onCompleted: currentIndex = brg.typesModel.valToIndex(boxData.type1);
                Connections {
                  target: boxData
                  function onType1Changed() { type1.currentIndex = brg.typesModel.valToIndex(boxData.type1); }
                }
                MainToolTip {
                  text: qsTr("The Pokémon's first type used for damage from incomming moves.")
                }
              }

              SelectType {
                id: theType2
                type2: true
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.maximumWidth: font.pixelSize * 8
                Layout.minimumWidth: font.pixelSize * 4
                Layout.preferredHeight: top.comboH

                onActivated: boxData.type2 = currentValue;
                Component.onCompleted: currentIndex = brg.typesModel.valToIndex(boxData.type2);
                Connections {
                  target: boxData
                  function onType2Changed() { theType2.currentIndex = brg.typesModel.valToIndex(boxData.type2); }
                }
                MainToolTip {
                  text: qsTr("The Pokémon's second type used for damage from incomming moves.")
                }
              }

              Item { Layout.fillWidth: true }

              // Randomize both types / reset to the species' DATABASE-default type(s)
              // (not literal Normal). update(resetType only) sets type1/type2 from the
              // species record; it no-ops on a glitch mon, so gate clear on validity.
              Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 30
                implicitWidth: typeGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: typeGrp
                  anchors.fill: parent
                  spacing: 0
                  SegBtn {
                    first: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
                    onClicked: top.randomizeTypes();
                    tip: qsTr("Roll random types.")
                  }
                  SegBtn {
                    icon.width: 16; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
                    enabled: boxData.isValidBool
                    onClicked: boxData.correctTypes();
                    tip: qsTr("Reset the typing to this species' default type(s).")
                  }
                }
              }
            }
          }

          // ---- OT Name + OT ID (grouped vertically; one shared Randomize/Clear pair,
          //      top-aligned with OT Name). A trade is defined by the OT name AND ID
          //      together, so the pair shares one Randomize (make/re-roll a phony
          //      trade) and one Clear (remove trade -> back to you). ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 86
            color: "transparent"

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              anchors.topMargin: 10
              anchors.bottomMargin: 10
              spacing: 12

              ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: 8

                RowLayout {
                  spacing: 12
                  RowLabel { text: "OT Name" }
                  DefTextEdit {
                    id: otNameEdit
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: font.pixelSize * 10
                    Layout.preferredHeight: top.textH

                    onTextChanged: boxData.otName = text;
                    Component.onCompleted: text = boxData.otName;
                    Connections {
                      target: boxData
                      function onOtNameChanged() { otNameEdit.text = boxData.otName; }
                    }
                    MainToolTip {
                      text: qsTr("This determines if the Pokémon is a traded Pokémon. If this doesn't match your character's Name and ID then it's considered not yours.")
                    }
                  }
                }

                RowLayout {
                  spacing: 12
                  RowLabel { text: "OT ID" }
                  DefTextEdit {
                    id: otIDEdit
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 4 * font.pixelSize + leftPadding + rightPadding
                    Layout.preferredHeight: top.textH
                    maximumLength: 4

                    onTextChanged: {
                      if(text === "")
                        return;

                      var idDec = parseInt(text, 16);
                      if(isNaN(idDec))
                        return;

                      if(idDec < 0 || idDec > 0xFFFF)
                        return;

                      boxData.otID = idDec;
                    }
                    Component.onCompleted: text = boxData.otID.toString(16).toUpperCase();
                    Connections {
                      target: boxData
                      function onOtIDChanged() { otIDEdit.text = boxData.otID.toString(16).toUpperCase(); }
                    }
                    MainToolTip {
                      text: qsTr("This determines if the Pokémon is a traded Pokémon. If this doesn't match your character's Name and ID then it's considered not yours.")
                    }
                  }
                }
              }

              Item { Layout.fillWidth: true }

              Rectangle {
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: 30
                implicitWidth: otGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: otGrp
                  anchors.fill: parent
                  spacing: 0
                  SegBtn {
                    first: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
                    onClicked: boxData.changeOtData(false);
                    tip: qsTr("Make this a traded Pokémon with a random OT name and ID.")
                  }
                  SegBtn {
                    icon.width: 16; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
                    enabled: boxData.otName === "phony" || boxData.otID === "phony" || boxData.hasTradeStatus(brg.file.data.dataExpanded.player.basics)
                    onClicked: boxData.changeOtData(true, brg.file.data.dataExpanded.player.basics);
                    tip: qsTr("Remove trade status — set the OT name and ID back to you.")
                  }
                }
              }
            }
          }

          // ---- Exp (the slider sits between Lv X and Lv X+1 markers to show it's
          //      the progress between those two levels). ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: top.rowAlt

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              spacing: 12

              RowLabel {
                text: (boxData.isValidBool) ? "Exp to Next Lvl" : "Exp"
              }

              ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 1

                // Lv markers framing the slider (valid mons only -- a glitch mon's
                // exp is a raw value, not between-levels progress, so they collapse).
                RowLayout {
                  Layout.fillWidth: true
                  visible: boxData.isValidBool
                  Text {
                    text: qsTr("Lv. ") + boxData.level
                    font.pixelSize: 11
                    color: brg.settings.textColorMid
                  }
                  Item { Layout.fillWidth: true }
                  Text {
                    text: (boxData.level < 100) ? qsTr("Lv. ") + (boxData.level + 1) : qsTr("Max")
                    font.pixelSize: 11
                    color: brg.settings.textColorMid
                  }
                }

                Slider {
                  id: nextExpEdit
                  Layout.fillWidth: true

                  enabled: boxData.level < 100 || !boxData.isValidBool

                  from: (boxData.isValidBool) ? boxData.expLevelRangeStart : 1
                  to: (boxData.isValidBool) ? boxData.expLevelRangeEnd : 0xFFFFFF

                  onMoved: boxData.exp = value;
                  Component.onCompleted: value = boxData.exp;

                  // Value shows on hover AND press (quick fade), per the slider standard.
                  ToolTip {
                    parent: nextExpEdit.handle
                    visible: nextExpEdit.pressed || nextExpEdit.hovered
                    text: nextExpEdit.value.toFixed(0)
                    Material.background: brg.settings.accentColor
                    Material.foreground: brg.settings.textColorLight
                    font.pixelSize: 14
                    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
                    exit:  Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
                  }

                  Connections {
                    target: boxData
                    function onExpChanged() { nextExpEdit.value = boxData.exp; }
                  }
                  MainToolTip {
                    text: qsTr("Fine-tune your Pokémon's exp between levels, for whole level changes change thel evel directly.")
                  }
                }
              }
            }
          }

          // ---- Catch Rate (Easy <-> Hard slider, 0..255). Higher catch-rate byte =
          //      easier to catch, so the slider is inverted (value = 255 - byte) to put
          //      Easy on the left as requested; the hover tooltip shows the real byte. ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "transparent"

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              spacing: 12

              RowLabel { text: "Catch Difficulty" }

              ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 1

                // Easy / Hard markers above the bar (like the Exp Lv markers).
                RowLayout {
                  Layout.fillWidth: true
                  Text {
                    text: qsTr("Easy")
                    font.pixelSize: 11
                    color: brg.settings.textColorMid
                  }
                  Item { Layout.fillWidth: true }
                  Text {
                    text: qsTr("Hard")
                    font.pixelSize: 11
                    color: brg.settings.textColorMid
                  }
                }

                Slider {
                  id: catchRateEdit
                  Layout.fillWidth: true
                  from: 0
                  to: 255
                  stepSize: 1

                  // Left (value 0) = byte 255 (Easy); right (value 255) = byte 0 (Hard).
                  onMoved: boxData.catchRate = 255 - value;
                  Component.onCompleted: value = 255 - boxData.catchRate;

                  ToolTip {
                    parent: catchRateEdit.handle
                    visible: catchRateEdit.pressed || catchRateEdit.hovered
                    text: (255 - catchRateEdit.value).toFixed(0)
                    Material.background: brg.settings.accentColor
                    Material.foreground: brg.settings.textColorLight
                    font.pixelSize: 14
                    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
                    exit:  Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
                  }

                  Connections {
                    target: boxData
                    function onCatchRateChanged() { catchRateEdit.value = 255 - boxData.catchRate; }
                  }
                  MainToolTip {
                    text: qsTr("Leftover battle data, not actually used by the game. Higher = easier to catch.")
                  }
                }
              }
            }
          }

          // ---- Future Nature ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: top.rowAlt

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              spacing: 12

              RowLabel { text: "Future Nature" }

              SelectNature {
                id: futureNatureEdit
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: font.pixelSize * 10
                Layout.preferredHeight: top.comboH

                onActivated: boxData.setNature(currentValue);
                Component.onCompleted: currentIndex = brg.natureSelectModel.natureToListIndex(boxData.getNature);
                Connections {
                  target: boxData
                  function onExpChanged() { futureNatureEdit.currentIndex = brg.natureSelectModel.natureToListIndex(boxData.getNature); }
                }
                MainToolTip {
                  text: qsTr("Nature's weren't created until gen 3, in the past few years Game Freak has released a formula for determining natures retroactively for Gen 1 games using IVs.")
                }
              }

              Item { Layout.fillWidth: true }

              // Randomize the (future) nature -- rolls a random nature, which rewrites
              // the DVs to produce it.
              Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 30
                implicitWidth: natGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: natGrp
                  anchors.fill: parent
                  spacing: 0
                  SegBtn {
                    first: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
                    onClicked: top.randomizeNature();
                    tip: qsTr("Roll a random nature.")
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
