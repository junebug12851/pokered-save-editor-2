import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../general"
import "../../header"
import "../../controls/selection"

Rectangle {
  id: top

  property PokemonBox boxData: null

  color: "transparent"

  ShadedBG {
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.bottom: parent.bottom
  }

  // Close details screen if the file data changes
  Connections {
    target: brg.file.data
    onDataExpandedChanged: brg.router.closeScreen();
  }

  Column {
    id: columnRoot

    anchors.top: parent.top
    anchors.left: parent.left

    width: parent.width

    Row {
      ShadedBG {height: 25}
    }

    Row {
      ShadedBG {
        id: nickNameHeader

        HeaderText {
          id: nickNameHeaderTxt
          text: "Nickname"
        }

        NameDisplay {
          id: monNameEdit

          anchors.top: nickNameHeaderTxt.top
          anchors.topMargin: 13
          anchors.left: nickNameHeader.right
          anchors.leftMargin: 15

          isPersonName: false
          isPlayerName: false

          sizeMult: nickNameHeaderTxt.font.pixelSize / 8

          onStrChanged: boxData.nickname = str;

          Connections {
            target: boxData
            onNicknameChanged: monNameEdit.str = boxData.nickname;
          }

          Component.onCompleted: monNameEdit.str = boxData.nickname;
        }

        IconButtonSquare {
          anchors.left: monNameEdit.right
          anchors.leftMargin: 20
          anchors.top: nickNameHeader.top
          anchors.topMargin: -4
          icon.width: 7

          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
          icon.color: brg.settings.textColorDark

          onClicked: nickNameMenu.open();

          Menu {
            id: nickNameMenu
            MenuItem { text: (boxData.hasNickname)
                             ? "Re-Roll Nickname"
                             : "Give Nickname"; onTriggered: boxData.changeName(false); }
            MenuItem { text: "Remove Nickname"; enabled: boxData.hasNickname;  onTriggered: boxData.changeName(true); }
            MenuSeparator { }
            MenuItem { text: "Close" }
          }
        }
      }
    }

    Row {
      ShadedBG {height: 10}
    }

    Row {
      ShadedBG {
        id: typeHeader

        HeaderText {
          text: "Type"
        }

        SelectType {
          id: type1
          anchors.top: typeHeader.top
          anchors.topMargin: -5
          anchors.left: typeHeader.right
          anchors.leftMargin: 5

          onActivated: boxData.type1 = currentValue;
          Component.onCompleted: currentIndex = brg.typesModel.valToIndex(boxData.type1);

          Connections {
            target: boxData
            onType1Changed: type1.currentIndex = brg.typesModel.valToIndex(boxData.type1);
          }
        }

        SelectType {
          id: theType2
          type2: true
          anchors.top: typeHeader.top
          anchors.topMargin: -5
          anchors.left: type1.right
          anchors.leftMargin: 7

          onActivated: boxData.type2 = currentValue;
          Component.onCompleted: currentIndex = brg.typesModel.valToIndex(boxData.type2);

          Connections {
            target: boxData
            onType2Changed: theType2.currentIndex = brg.typesModel.valToIndex(boxData.type2);
          }
        }
      }
    }

    Row {
      ShadedBG {
        id: otName

        HeaderText {
          text: "OT Name"
        }

        DefTextEdit {
          id: otNameEdit
          anchors.top: otName.top
          anchors.topMargin: 9
          anchors.left: otName.right
          anchors.leftMargin: 17

          width: type1.width - 15

          onTextChanged: boxData.otName = text;
          Component.onCompleted: text = boxData.otName;

          Connections {
            target: boxData
            onOtNameChanged: otNameEdit.text = boxData.otName;
          }
        }

        IconButtonSquare {
          anchors.left: otNameEdit.right
          anchors.leftMargin: -4
          anchors.top: otName.top
          anchors.topMargin: 0
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
    }

    Row {
      ShadedBG {
        id: otID

        HeaderText {
          text: "OT ID"
        }

        DefTextEdit {
          id: otIDEdit
          anchors.top: otID.top
          anchors.topMargin: 9
          anchors.left: otID.right
          anchors.leftMargin: 17

          width: type1.width - 15
          maximumLength: 4

          onTextChanged: {
            if(text === "")
              return;

            var idDec = parseInt(text, 16);
            if(idDec === NaN)
              return;

            if(idDec < 0 || idDec > 0xFFFF)
              return;

            boxData.otID = idDec;
          }
          Component.onCompleted: text = boxData.otID.toString(16).toUpperCase();

          Connections {
            target: boxData
            onOtIDChanged: otIDEdit.text = boxData.otID.toString(16).toUpperCase();
          }
        }

        IconButtonSquare {
          anchors.left: otIDEdit.right
          anchors.leftMargin: -4
          anchors.top: otID.top
          anchors.topMargin: 0
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
    }

    Row {
      ShadedBG {
        id: nextExp

        HeaderText {
          text: (boxData.isValidBool)
                ? "Exp to Next Lvl"
                : "Exp"
        }

        Slider {
          id: nextExpEdit

          enabled: boxData.level < 100 || !boxData.isValidBool

          anchors.top: nextExp.top
          anchors.topMargin: -5
          anchors.left: nextExp.right
          anchors.leftMargin: 12

          width: top.width - nextExp.width - (25 * 2)

          from: (boxData.isValidBool)
                ? boxData.expLevelRangeStart
                : 1
          to: (boxData.isValidBool)
              ? boxData.expLevelRangeEnd
              : 0xFFFFFF

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
            onExpChanged: nextExpEdit.value = boxData.exp;
          }
        }
      }
    }

    Row {
      ShadedBG {
        id: catchRate

        HeaderText {
          text: "Catch Rate"
        }

        DefTextEdit {
          id: catchRateEdit
          anchors.top: catchRate.top
          anchors.topMargin: 9
          anchors.left: catchRate.right
          anchors.leftMargin: 17

          width: type1.width - 15

          maximumLength: 3

          onTextChanged: {
            if(text === "")
              return;

            var idDec = parseInt(text, 10);
            if(idDec === NaN)
              return;

            if(idDec < 0 || idDec > 0xFF)
              return;

            boxData.catchRate = idDec;
          }
          Component.onCompleted: text = boxData.catchRate.toString(10);

          Connections {
            target: boxData
            onCatchRateChanged: catchRateEdit.text = boxData.catchRate.toString(10);
          }
        }
      }
    }

    Row {
      ShadedBG {
        id: futureShiny

        HeaderText {
          text: "Future Shiny"
        }

        CheckBox {
          id: futureShinyEdit

          anchors.top: futureShiny.top
          anchors.topMargin: -5
          anchors.left: futureShiny.right
          anchors.leftMargin: 6

          onCheckedChanged: {
            if(checked)
              boxData.makeShiny();
            else
              boxData.unmakeShiny();
          }

          Component.onCompleted: checked = boxData.isShiny;

          Connections {
            target: boxData
            onDvChanged: futureShinyEdit.checked = boxData.isShiny;
          }
        }

        IconButtonSquare {
          anchors.left: futureShinyEdit.right
          anchors.leftMargin: -15
          anchors.top: futureShiny.top
          anchors.topMargin: -3
          icon.width: 7

          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
          icon.color: brg.settings.textColorDark

          onClicked: futureShinyMenu.open();

          Menu {
            id: futureShinyMenu
            MenuItem {
              text: "Re-Roll Shiny";
              onTriggered: boxData.rollShiny();
            }
            MenuItem {
              text: "Re-Roll Non-Shiny";
              onTriggered: boxData.rollNonShiny();
            }
            MenuSeparator { }
            MenuItem {
              text: "Convert to Shiny"
              enabled: !boxData.isShiny
              onTriggered: boxData.makeShiny();
            }
            MenuItem {
              text: "Convert to Non-Shiny"
              enabled: boxData.isShiny
              onTriggered: boxData.unmakeShiny();
            }
            MenuSeparator { }
            MenuItem { text: "Close" }
          }
        }
      }
    }
  }
}
