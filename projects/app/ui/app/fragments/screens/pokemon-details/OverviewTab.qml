import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../general"
import "../../header"
import "../../controls/selection"

// General tab. Each row is a RowLayout of [shaded label box | control(s) | …]:
//  - the label box (Layout.fillHeight) grows to the row's field height so the
//    shaded strip and field read as one aligned row (Twilight's option #2),
//  - a flexible Item { Layout.fillWidth: true } sits BEFORE any ⋮ menu button so
//    the dots are right-aligned to the row edge regardless of the field width,
//  - no fixed/negative offsets.
// Wrapped in a ScrollView so rows lay out at full height and scroll if long.
Rectangle {
  id: top
  color: "transparent"

  property PokemonBox boxData: null

  // Field heights: text boxes are shorter, combos a touch taller (Twilight's tuning).
  // One knob per type keeps every row consistent.
  property int textH: 30
  property int comboH: 38

  // Close details screen if the file data changes
  Connections {
    target: brg.file.data
    function onDataExpandedChanged() { brg.router.closeScreen(); }
  }

  // Shared look for the left shaded label cell.
  component FieldLabel: Rectangle {
    property alias text: labelText.text
    property alias labelEl: labelText
    Layout.preferredWidth: 110
    Layout.fillHeight: true
    color: Qt.lighter(brg.settings.textColorMid, 1.75)

    HeaderText { id: labelText }
  }

  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth

    ColumnLayout {
      // Reserve room on the right for the (overlay) scrollbar so the right-
      // aligned ⋮ menu buttons sit clear of it and stay clickable.
      width: scroller.availableWidth - 16
      spacing: 4

      // ---- Nickname ----
      RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 8
        spacing: 8

        FieldLabel { id: nickNameLabel; text: "Nickname" }

        NameDisplay {
          id: monNameEdit
          Layout.alignment: Qt.AlignVCenter

          isPersonName: false
          isPlayerName: false
          sizeMult: nickNameLabel.labelEl.font.pixelSize / 8

          onStrChanged: boxData.nickname = str;
          Connections {
            target: boxData
            function onNicknameChanged() { monNameEdit.str = boxData.nickname; }
          }
          Component.onCompleted: monNameEdit.str = boxData.nickname;
        }

        Item { Layout.fillWidth: true }

        IconButtonSquare {
          Layout.alignment: Qt.AlignVCenter
          icon.width: 7
          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
          icon.color: brg.settings.textColorDark

          onClicked: nickNameMenu.open();

          Menu {
            id: nickNameMenu
            MenuItem { text: (boxData.hasNickname || !boxData.isValidBool)
                             ? "Re-Roll Nickname"
                             : "Give Nickname"; onTriggered: boxData.changeName(false); }
            MenuItem { text: "Remove Nickname"; enabled: boxData.hasNickname; onTriggered: boxData.changeName(true); }
            MenuSeparator { }
            MenuItem { text: "Close" }
          }

          MainToolTip {
            text: "Pokemon's display name, it's considered to be a Nickname if it isn't the species name in all caps. Nickname's are difficult to change and are unaffected by evolution."
          }
        }
      }

      // ---- Type ----
      RowLayout {
        Layout.fillWidth: true
        spacing: 8

        FieldLabel { text: "Type" }

        SelectType {
          id: type1
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredWidth: font.pixelSize * 8
          Layout.preferredHeight: top.comboH

          onActivated: boxData.type1 = currentValue;
          Component.onCompleted: currentIndex = brg.typesModel.valToIndex(boxData.type1);
          Connections {
            target: boxData
            function onType1Changed() { type1.currentIndex = brg.typesModel.valToIndex(boxData.type1); }
          }
          MainToolTip {
            text: "The Pokemon's first type used for damage from incomming moves."
          }
        }

        SelectType {
          id: theType2
          type2: true
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredWidth: font.pixelSize * 8
          Layout.preferredHeight: top.comboH

          onActivated: boxData.type2 = currentValue;
          Component.onCompleted: currentIndex = brg.typesModel.valToIndex(boxData.type2);
          Connections {
            target: boxData
            function onType2Changed() { theType2.currentIndex = brg.typesModel.valToIndex(boxData.type2); }
          }
          MainToolTip {
            text: "The Pokemon's second type used for damage from incomming moves."
          }
        }

        Item { Layout.fillWidth: true }
      }

      // ---- OT Name ----
      RowLayout {
        Layout.fillWidth: true
        spacing: 8

        FieldLabel { text: "OT Name" }

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
            text: "This determines if the Pokemon is a traded Pokemon. If this doesn't match your character's Name and ID then it's considered not yours."
          }
        }

        Item { Layout.fillWidth: true }

        IconButtonSquare {
          Layout.alignment: Qt.AlignVCenter
          icon.width: 7
          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
          icon.color: brg.settings.textColorDark

          onClicked: otNameMenu.open();

          Menu {
            id: otNameMenu
            MenuItem {
              text: (boxData.otName === "phony" || boxData.otID === "phony" || boxData.hasTradeStatus(brg.file.data.dataExpanded.player.basics))
                    ? "Re-Roll Trade"
                    : "Make Trade";
              onTriggered: boxData.changeOtData(false);
            }
            MenuItem {
              text: "Remove Trade";
              enabled: boxData.otName === "phony" || boxData.otID === "phony" || boxData.hasTradeStatus(brg.file.data.dataExpanded.player.basics);
              onTriggered: boxData.changeOtData(true, brg.file.data.dataExpanded.player.basics);
            }
            MenuSeparator { }
            MenuItem { text: "Close" }
          }
        }
      }

      // ---- OT ID (4 hex chars) ----
      RowLayout {
        Layout.fillWidth: true
        spacing: 8

        FieldLabel { text: "OT ID" }

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
            text: "This determines if the Pokemon is a traded Pokemon. If this doesn't match your character's Name and ID then it's considered not yours."
          }
        }

        Item { Layout.fillWidth: true }

        IconButtonSquare {
          Layout.alignment: Qt.AlignVCenter
          icon.width: 7
          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
          icon.color: brg.settings.textColorDark

          onClicked: otIdMenu.open();

          Menu {
            id: otIdMenu
            MenuItem {
              text: (boxData.otName === "phony" || boxData.otID === "phony" || boxData.hasTradeStatus(brg.file.data.dataExpanded.player.basics))
                    ? "Re-Roll Trade"
                    : "Make Trade";
              onTriggered: boxData.changeOtData(false);
            }
            MenuItem {
              text: "Remove Trade";
              enabled: boxData.otName === "phony" || boxData.otID === "phony" || boxData.hasTradeStatus(brg.file.data.dataExpanded.player.basics);
              onTriggered: boxData.changeOtData(true, brg.file.data.dataExpanded.player.basics);
            }
            MenuSeparator { }
            MenuItem { text: "Close" }
          }
        }
      }

      // ---- Exp ----
      RowLayout {
        Layout.fillWidth: true
        spacing: 8

        FieldLabel {
          text: (boxData.isValidBool) ? "Exp to Next Lvl" : "Exp"
        }

        Slider {
          id: nextExpEdit
          Layout.fillWidth: true
          Layout.rightMargin: 25
          Layout.preferredHeight: top.textH
          Layout.alignment: Qt.AlignVCenter

          enabled: boxData.level < 100 || !boxData.isValidBool

          from: (boxData.isValidBool) ? boxData.expLevelRangeStart : 1
          to: (boxData.isValidBool) ? boxData.expLevelRangeEnd : 0xFFFFFF

          onMoved: boxData.exp = value;
          Component.onCompleted: value = boxData.exp;

          ToolTip {
            parent: nextExpEdit.handle
            visible: nextExpEdit.pressed
            text: nextExpEdit.value.toFixed(0)
            Material.background: brg.settings.accentColor
            Material.foreground: brg.settings.textColorLight
            font.pixelSize: 14
          }

          Connections {
            target: boxData
            function onExpChanged() { nextExpEdit.value = boxData.exp; }
          }
          MainToolTip {
            text: "Fine-tune your Pokemon's exp between levels, for whole level changes change thel evel directly."
          }
        }
      }

      // ---- Catch Rate (same width as OT ID) ----
      RowLayout {
        Layout.fillWidth: true
        spacing: 8

        FieldLabel { text: "Catch Rate" }

        DefTextEdit {
          id: catchRateEdit
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredWidth: 4 * font.pixelSize + leftPadding + rightPadding
          Layout.preferredHeight: top.textH
          maximumLength: 3

          onTextChanged: {
            if(text === "")
              return;

            var idDec = parseInt(text, 10);
            if(isNaN(idDec))
              return;

            if(idDec < 0 || idDec > 0xFF)
              return;

            boxData.catchRate = idDec;
          }
          Component.onCompleted: text = boxData.catchRate.toString(10);
          Connections {
            target: boxData
            function onCatchRateChanged() { catchRateEdit.text = boxData.catchRate.toString(10); }
          }
          MainToolTip {
            text: "This isn't used at all, it's leftover garbage from when you were battling the Pokemon. Nonetheless it does exist and you can edit it here."
          }
        }

        Item { Layout.fillWidth: true }
      }

      // ---- Future Nature ----
      RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: 8
        spacing: 8

        FieldLabel { text: "Future Nature" }

        SelectNature {
          id: futureNatureEdit
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredWidth: font.pixelSize * 10
          Layout.preferredHeight: top.comboH

          onActivated: boxData.setNature(currentValue);
          Component.onCompleted: currentIndex = brg.natureSelectModel.natureToListIndex(boxData.getNature)